#include "so_lib.h"

#include <dlfcn.h>
#include <dyncall.h>

#include <set>

#include "common.h"
#include "ucenv/ucenv.h"

SoLib::~SoLib()
{
	if (dlclose(so_)) {
		ABORT("dlclose failed: " + path_ + ": " + std::string(dlerror()));
	}
}

SoLib::SoLib(const std::string &so_path)
	: path_(so_path)
{
	so_=dlopen(path_.c_str(), RTLD_NOW);
	if (!so_) {
		ABORT("dlopen failed: " + path_ + ": " + std::string(dlerror()));
	}
}

ValuePtr df_to_value(const std::shared_ptr<DF> &df)
{
	if (df->getType()==DF::Unset) {
		ABORT("Unexpected unset df: " + std::string(df->getCName()));
	} else if (df->getType()==DF::Int) {
		return ValuePtr(new IntValue(df->getInt()));
	} else if (df->getType()==DF::Real) {
		return ValuePtr(new RealValue(df->getReal()));
	} else if (df->getType()==DF::String) {
		return ValuePtr(new StringValue(df->getString()));
	} else if (df->getType()==DF::Value) {
		void *data;
		size_t size;
		std::tie(data, size)=df->grabBuffer();
		return ValuePtr(CustomValue::create_take(data, size, [data](){
			operator delete(data);
		}));
	} else {
		ABORT("Unexpected DF type: " + std::to_string(df->getType()));
	}
}

void SoLib::execute(const std::string &code, 
	std::vector<CodeLib::Argument> &args)
{
	void *func=dlsym(so_, code.c_str());
	if (!func) {
		ABORT("dlsym failed: " + path_ + ": " + code + ": "
			+ std::string(dlerror()));
	}

	DCCallVM *vm=dcNewCallVM(4096);
	assert(vm);

	dcMode(vm, DC_CALL_C_DEFAULT);
	dcReset(vm);

	std::vector<std::shared_ptr<DF> > dfargs;
	std::vector<std::shared_ptr<std::string> > strings;

	for (auto i=0u; i<args.size(); i++) {
		if (std::get<0>(args[i])==Reference) {
			auto df=new DF(std::get<2>(args[i]).to_string());
			dfargs.push_back(std::shared_ptr<DF>(df));
			dcArgPointer(vm, df);
		} else if (std::get<0>(args[i])==Custom) {
			void *data;
			size_t size;
			ValuePtr val=std::get<1>(args[i]);
			CustomValue *cval=dynamic_cast<CustomValue*>(val.get());
			std::tie(data, size)=cval->grab_buffer();
			auto df=new DF(std::get<2>(args[i]).to_string(),
				data, size);
			dfargs.push_back(std::shared_ptr<DF>(df));
			dcArgPointer(vm, df);
		} else {
			ValuePtr val=std::get<1>(args[i]);
			if (std::get<0>(args[i])==Integer) {
				dcArgInt(vm, (int)(*val));
			} else if (std::get<0>(args[i])==Real) {
				dcArgDouble(vm, (double)(*val));
			} else if (std::get<0>(args[i])==String) {
				std::shared_ptr<std::string> s(
					new std::string((std::string)(*val)));
				dcArgPointer(vm, const_cast<char*>(s->c_str()));
				strings.push_back(s); // to prevent memory deletion
			} else {
				ABORT("Unknown type: " + std::to_string(
					std::get<0>(args[i])));
			}
			dfargs.push_back(std::shared_ptr<DF>(nullptr));
				// to preserve numeration
		}
	}

	dcCallVoid(vm, (DCpointer)func);
	dcFree(vm);

	for (auto i=0u; i<args.size(); i++) {
		if (std::get<0>(args[i])==Reference) {
			auto val=df_to_value(dfargs[i]);
			args[i]=std::make_tuple(
				Reference,
				val,
				std::get<2>(args[i]));
		} else if (std::get<0>(args[i])==Custom) {
			if (!dfargs[i]->isSet()) {
				// grabbed out: unset ValuePtr
				args[i]=std::make_tuple(
					Custom,
					ValuePtr(nullptr),
					std::get<2>(args[i]));
			}
		} else {
			// Constant argument, do nothing
		}
	}
}

