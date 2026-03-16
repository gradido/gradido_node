#include "fromJson.h"
#include "WireFilter.h"
#include "gradido_blockchain/AppContext.h"
#include "gradido_blockchain/blockchain/CompactFilter.h"
#include "gradido_blockchain/blockchain/Pagination.h"
#include "gradido_blockchain/blockchain/SearchDirection.h"
#include "gradido_blockchain/CommunityContext.h"
#include "gradido_blockchain/crypto/ByteArray.h"
#include "gradido_blockchain/data/compact/PublicKeyIndex.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"
#include "gradido_blockchain/lib/TimepointInterval.h"

#include <magic_enum/magic_enum.hpp>
#include <rapidjson/document.h>
#include <sodium.h>

#include <optional>
#include <string>
#include <string_view>

using namespace magic_enum;
using DataTypeConverter::dateTimeStringToTimePoint;
using gradido::AppContext;
using gradido::blockchain::CompactFilter, gradido::blockchain::Pagination, gradido::blockchain::SearchDirection;
using gradido::CommunityContext;
using gradido::data::compact::PublicKeyIndex;
using rapidjson::Value;
using std::string, std::string_view;

namespace server::json_rpc {

  static JsonParseResult checkFieldName(const Value& json, const char* fieldName)
  {
    if (!json.HasMember(fieldName)) return { .type = JsonParseResultType::Missing_Field };
    if (json[fieldName].IsNull()) return { .type = JsonParseResultType::Null_Field };
    return { .type = JsonParseResultType::Ok };
  }

  static JsonParseResult isNonEmptyStringField(const Value& json, const char* fieldName)
  {
    auto result = checkFieldName(json, fieldName);
    if (result.type != JsonParseResultType::Ok) return result;
    if (!json[fieldName].IsString()) {
      string errorMessage = "expect string type for field: ";
      errorMessage += fieldName;
      return {
        .type = JsonParseResultType::Type_Mismatch,
        .error = errorMessage
      };
    }
    return { .type = JsonParseResultType::Ok };
  }

  static JsonParseResult isNonEmptyNumericalField(const Value& json, const char* fieldName)
  {
    auto result = checkFieldName(json, fieldName);
    if (result.type != JsonParseResultType::Ok) return result;
    if (!json[fieldName].IsNumber()) {
      string errorMessage = "expect number type for field: ";
      errorMessage += fieldName;
      return {
        .type = JsonParseResultType::Type_Mismatch,
        .error = errorMessage
      };
    }
    return { .type = JsonParseResultType::Ok };
  }

  static JsonParseResult isNonEmptyObject(const Value& json, const char* fieldName)
  {
    auto result = checkFieldName(json, fieldName);
    if (result.type != JsonParseResultType::Ok) return result;
    if (!json[fieldName].IsObject()) {
      string errorMessage = "expect object type for field: ";
      errorMessage += fieldName;
      return {
        .type = JsonParseResultType::Type_Mismatch,
        .error = errorMessage
      };
    }
    return { .type = JsonParseResultType::Ok };
  }
  
  // enum
  template<typename T>
  requires std::is_enum_v<T>
  JsonParseResult fromJson(const Value& json, const char* fieldName, T& value) 
  {
    auto result = isNonEmptyStringField(json, fieldName);
    if (result.type != JsonParseResultType::Ok) return result;

    string_view valueString(json[fieldName].GetString(), json[fieldName].GetStringLength());
    auto optionalValue = enum_cast<T>(valueString);
    if (!optionalValue) 
    {
      string errorMessage = fieldName;
      errorMessage += ": ";
      errorMessage += valueString;
      errorMessage += " is invalid for enum ";
      errorMessage += enum_type_name<T>();
      return {
        .type = JsonParseResultType::Invalid_Enum,
        .error = errorMessage
      };
    }
    value = optionalValue.value();
    return { .type = JsonParseResultType::Ok };
  }

  // unsigned integer
  static JsonParseResult fromJson(const Value& json, const char* fieldName, uint32_t& value) 
  {
    auto result = isNonEmptyNumericalField(json, fieldName);
    if (result.type != JsonParseResultType::Ok) return result;

    value = json[fieldName].GetUint();
    return { .type = JsonParseResultType::Ok };
  }

  // unsigned long long
  static JsonParseResult fromJson(const Value& json, const char* fieldName, uint64_t& value)
  {
    auto result = isNonEmptyNumericalField(json, fieldName);
    if (result.type != JsonParseResultType::Ok) return result;

    value = json[fieldName].GetUint64();
    return { .type = JsonParseResultType::Ok };
  }

  // string
  static JsonParseResult fromJson(const Value& json, const char* fieldName, std::string& value)
  {
    auto result = isNonEmptyStringField(json, fieldName);
    if (result.type != JsonParseResultType::Ok) return result;

    value = string(json[fieldName].GetString(), json[fieldName].GetStringLength());
    return { .type = JsonParseResultType::Ok };
  }

  // Timepoint
  static JsonParseResult fromJson(const Value& json, const char* fieldName, Timepoint& value)
  {
    auto result = isNonEmptyStringField(json, fieldName);
    if (result.type != JsonParseResultType::Ok) return result;

    // expect JavaScript DateTime String
    // TODO: allow more formats, maybe with fmtLib
    value = dateTimeStringToTimePoint(string(json[fieldName].GetString(), json[fieldName].GetStringLength()), "%FT%F");
    return { .type = JsonParseResultType::Ok };
  }

  // PublicKey
  static JsonParseResult fromJson(const Value& json, const char* fieldName, PublicKey& publicKey)
  {
    auto result = isNonEmptyStringField(json, fieldName);
    if (result.type != JsonParseResultType::Ok) return result;

    string_view valueString(json[fieldName].GetString(), json[fieldName].GetStringLength());
    if (valueString.size() != 65) {
      string errorMessage = fieldName;
      errorMessage += " hasn't expected size of 64 hex character + string end character";
      return {
        .type = JsonParseResultType::Invalid_Field_Value,
        .error = errorMessage
      };
    }
    
    size_t resultBinSize = 0;
    unsigned char buffer[32];
    if (0 != sodium_hex2bin(buffer, 32, valueString.data(), valueString.size(), nullptr, &resultBinSize, nullptr)) {
      string errorMessage = fieldName;
      errorMessage += " contain invalid hex";
      return {
        .type = JsonParseResultType::Invalid_Hex,
        .error = errorMessage
      };
    }
    
    publicKey = PublicKey(buffer);
    return { .type = JsonParseResultType::Ok };
  }

  // Pagination
  static JsonParseResult fromJson(const Value& json, const char* fieldName, Pagination& pagination)
  {
    auto result = isNonEmptyObject(json, fieldName);
    if (result.type != JsonParseResultType::Ok) return result;

    const auto& valuePagination = json[fieldName];
    // size
    auto sizeResult = isNonEmptyNumericalField(valuePagination, "size");
    if (JsonParseResultType::Type_Mismatch == sizeResult.type) {
      return sizeResult;
    }
    if (JsonParseResultType::Ok == sizeResult.type) {
      pagination.size = valuePagination["size"].GetUint();
    }
    if (pagination.size > 100) {
      pagination.size = 100;
    }
    // page
    auto pageResult = isNonEmptyNumericalField(valuePagination, "page");
    if (JsonParseResultType::Type_Mismatch == pageResult.type) {
      return pageResult;
    }
    if (JsonParseResultType::Ok == pageResult.type) {
      pagination.page = valuePagination["page"].GetUint();
    }
    return { .type = JsonParseResultType::Ok };
  }

  // TimepointInterval
  static JsonParseResult fromJson(const Value& json, const char* fieldName, TimepointInterval& timepointInterval)
  {
    auto result = isNonEmptyObject(json, fieldName);
    if (result.type != JsonParseResultType::Ok) return result;

    const auto& valueTimepointIterval = json[fieldName];
    // start date
    Timepoint tempTimepoint;
    auto startDateResult = fromJson(valueTimepointIterval, "startDate", tempTimepoint);
    if (JsonParseResultType::Ok == startDateResult.type) {
      timepointInterval.setStartDate(tempTimepoint);
    }
    else if (JsonParseResultType::Type_Mismatch == startDateResult.type) {
      return startDateResult;
    }
    // end date
    auto endDateResult = fromJson(valueTimepointIterval, "endDate", tempTimepoint);
    if (JsonParseResultType::Ok == endDateResult.type) {
      timepointInterval.setEndDate(tempTimepoint);
    }
    else if (JsonParseResultType::Type_Mismatch == endDateResult.type) {
      return endDateResult;
    }
    return { .type = JsonParseResultType::Ok };
  }

  // WireFilter
  JsonParseResult fromJson(const Value& value, WireFilter& filter)
  {
    auto result = fromJson(value, "searchDirection", filter.searchDirection);
    if (JsonParseResultType::Invalid_Enum == result.type || JsonParseResultType::Type_Mismatch == result.type) return result;

    result = fromJson(value, "transactionType", filter.transactionType);
    if (JsonParseResultType::Invalid_Enum == result.type || JsonParseResultType::Type_Mismatch == result.type) return result;

    result = fromJson(value, "publicKeySearchType", filter.publicKeySearchType);
    if (JsonParseResultType::Invalid_Enum == result.type || JsonParseResultType::Type_Mismatch == result.type) return result;

    result = fromJson(value, "format", filter.format);
    if (JsonParseResultType::Invalid_Enum == result.type || JsonParseResultType::Type_Mismatch == result.type) return result;

    result = fromJson(value, "communityId", filter.communityId);
    if (JsonParseResultType::Type_Mismatch == result.type) return result;

    result = fromJson(value, "coinCommunityId", filter.coinCommunityId);
    if (JsonParseResultType::Type_Mismatch == result.type) return result;

    result = fromJson(value, "maxTransactionNr", filter.maxTransactionNr);
    if (JsonParseResultType::Type_Mismatch == result.type) return result;

    result = fromJson(value, "minTransactionNr", filter.minTransactionNr);
    if (JsonParseResultType::Type_Mismatch == result.type) return result;
    
    result = fromJson(value, "publicKey", filter.publicKey);
    if (
      JsonParseResultType::Invalid_Field_Value == result.type || 
      JsonParseResultType::Invalid_Hex == result.type ||
      JsonParseResultType::Type_Mismatch == result.type
    ) { 
      return result; 
    }

    result = fromJson(value, "pagination", filter.pagination);
    if (JsonParseResultType::Type_Mismatch == result.type) return result;

    result = fromJson(value, "timepointInterval", filter.timepointInterval);
    if (JsonParseResultType::Type_Mismatch == result.type) return result;

    return { .type = JsonParseResultType::Ok };
  }
  
}