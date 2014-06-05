#include "candidate-factory.h"
#include "candidate-efl.h"
#include "candidate-multiline-efl.h"

Candidate *
CandidateFactory::make_candidate(CandidateType type,
    void* parent) {
    switch (type) {
        case CANDIDATE_SINGLE:
            return new EflCandidate((Evas_Object *)parent);
        case CANDIDATE_MULTILINE:
            return new EflMultiLineCandidate((Evas_Object *)parent);
        default:
            return NULL;
    }
}
