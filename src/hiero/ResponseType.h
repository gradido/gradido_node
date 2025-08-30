#ifndef __GRADIDO_NODE_HIERO_RESPONSE_TYPE_H
#define __GRADIDO_NODE_HIERO_RESPONSE_TYPE_H

namespace hiero {
    enum class ResponseType {
        /**
        * A response with the query answer.
        */
        ANSWER_ONLY = 0,

        /**
        * A response with both the query answer and a state proof.
        */
        ANSWER_STATE_PROOF = 1,

        /**
        * A response with the estimated cost to answer the query.
        */
        COST_ANSWER = 2,

        /**
        * A response with the estimated cost to answer and a state proof.
        */
        COST_ANSWER_STATE_PROOF = 3
    };
}

#endif