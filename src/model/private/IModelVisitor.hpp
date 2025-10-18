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

    virtual void visitRegularState(class RegularState* state) = 0;
    virtual void visitEntryPoint(class EntryPoint* entryPoint) = 0;
    virtual void visitExitPoint(class ExitPoint* exitPoint) = 0;
    virtual void visitFinalState(class FinalState* finalState) = 0;
    virtual void visitHistoryState(class HistoryState* historyState) = 0;
    virtual void visitInitialState(class InitialState* historyState) = 0;
    virtual void visitTransition(class Transition* transition) = 0;
};

}  // namespace model

#endif  // IMODELVISITOR_HPP