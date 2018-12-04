#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <map>
#include <memory>

#include "id.h"
#include "value.h"

typedef std::map<Name, Value> Context;
typedef std::map<Id, ValuePtr> Dfs;

typedef std::shared_ptr<Context> ContextPtr;
typedef std::shared_ptr<Dfs> DfsPtr;

#endif // CONTEXT_H_
