#ifndef IMODELVISITOR_HPP
#define IMODELVISITOR_HPP

namespace model {

class RegularState;
class EntryPoint;
class ExitPoint;
class FinalState;
class HistoryState;
class InitialState;
class Transition;

class IModelVisitor {
public:
    virtual ~IModelVisitor() = default;

    virtual void visitRegularState(const RegularState* state) = 0;
    virtual void visitEntryPoint(const EntryPoint* entryPoint) = 0;
    virtual void visitExitPoint(const ExitPoint* exitPoint) = 0;
    virtual void visitFinalState(const FinalState* finalState) = 0;
    virtual void visitHistoryState(const HistoryState* historyState) = 0;
    virtual void visitInitialState(const InitialState* historyState) = 0;
    virtual void visitTransition(const Transition* transition) = 0;
};

}  // namespace model

#endif  // IMODELVISITOR_HPP