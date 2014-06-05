#ifndef __CANDIDATE_FACTORY_H__
#define __CANDIDATE_FACTORY_H__
#include "candidate.h"
// simple factory pattern
enum CandidateType {
    CANDIDATE_SINGLE,
    CANDIDATE_MULTILINE
};

class CandidateFactory {
public:
    static Candidate *make_candidate(CandidateType type,
        void* parent);
};

#endif
