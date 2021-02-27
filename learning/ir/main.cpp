#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include <iostream>

// LLVM 8.0.1
 
using namespace llvm;

ExecutionEngine* ee = NULL;
RTDyldMemoryManager* ldMM = NULL;

void initEE(std::unique_ptr<Module> Owner) {
    std::string errStr;
    if (ee == NULL) {
        ldMM = new SectionMemoryManager();
        ee = EngineBuilder(std::move(Owner))
            .setEngineKind(EngineKind::JIT)
            .setErrorStr(&errStr)
            .setVerifyModules(true)
            .setMCJITMemoryManager(std::unique_ptr<RTDyldMemoryManager>(ldMM))
            .setOptLevel(CodeGenOpt::Default)
            .create();

    } else
        ee->addModule(std::move(Owner));
    if (errStr.length() != 0)
        std::cerr << "Create Engine Error" << std::endl << errStr << std::endl;
    ee->finalizeObject();
}

int main(){
	static LLVMContext MyGlobalContext;
  LLVMContext &context = MyGlobalContext;
	
	//创建一个module
	Module *module = new Module("test", context);

	//声明一个函数-----------------------------------
	SmallVector<Type *, 2> functionArgs;
	//各种类型都是通过get方法来构造对应的实例
    functionArgs.push_back(Type::getInt32Ty(context)); //32为整型参数Tina及到vector数组中
    functionArgs.push_back(Type::getInt32Ty(context));
	Type *returnType = Type::getInt32Ty(context);//返回值类型
	FunctionType *max_type = FunctionType::get(returnType, functionArgs, /*isVarArg*/ false);

	//声明 int max(int,a int b); 函数
	//cast将指针或者引用从基类转向为派生类，可以从 http://llvm.org/docs/ProgrammersManual.html#the-c-standard-template-library 这个链接学习
	Function *max_fun = cast<Function>(module->getOrInsertFunction("max", max_type));//将函数插入module

	//函数体-----------------------------------------------
	//存储参数（获取参数的引用）
	Function::arg_iterator argsIT = max_fun->arg_begin();//Function中的一个方法，获取参数的迭代器
	Value *arg_a = argsIT++;//获取第一个参数
	arg_a->setName("a");//设置第一个参数名为a
	Value *arg_b = argsIT++;
	arg_b->setName("b");

	//创建max函数的entry代码块
	BasicBlock *entry_max = BasicBlock::Create(context, "entry", max_fun);
	IRBuilder<> builder_max(entry_max);//创建builder实例，每个basicblock对应一个irbulider

	//比较arg_a和arg_b的大小
	Value *cmp_value = builder_max.CreateICmpSGT(arg_a, arg_b);

	//创建一个if.then代码块
	BasicBlock *if_then = BasicBlock::Create(context, "if_then", max_fun);
	IRBuilder<> builder_then(if_then);

	//创建一个if.else代码块
	BasicBlock *if_else = BasicBlock::Create(context, "if_else", max_fun);
	IRBuilder<> builder_else(if_else);

	//根据cmp的值跳转，也就是if条件
	builder_max.CreateCondBr(cmp_value, if_then, if_else);

	//创建返回值
	builder_then.CreateRet(arg_a);
	builder_else.CreateRet(arg_b);
	//-----------------------------------------------------

	//声明 int main() 函数----------------------------------
	SmallVector<Type *, 1> mainFunctionArgs;
    //各种类型都是通过get方法来构造对应的实例
    // mainFunctionArgs.push_back(Type::getVoidTy(context)); // void类型不需要push设置类型
	Type *mainReturnType = Type::getInt32Ty(context);//返回值类型
	FunctionType *main_type = FunctionType::get(mainReturnType, mainFunctionArgs, /*isVarArg*/ false);

	// FunctionType *main_type = TypeBuilder<int(), false>::get(context);
	Function *main_fun = cast<Function>(module->getOrInsertFunction("main", main_type));

	//创建main函数的entry代码块
	BasicBlock *entry_main = BasicBlock::Create(context, "entry", main_fun);
	IRBuilder<> builder_main(entry_main);

	//创建一个i32常量
	Value *a_value = ConstantInt::get(Type::getInt32Ty(context), -10);
	Value *b_value = ConstantInt::get(Type::getInt32Ty(context),20);
	
	//构造实参列表
	std::vector<Value*> putsargs;
	putsargs.push_back(a_value);
	putsargs.push_back(b_value);
	ArrayRef<Value*>  argsRef(putsargs);

	// 调用函数max
	Value *ret = builder_main.CreateCall(max_fun, argsRef);
	
	//创建返回值
	builder_main.CreateRet(ret);
	//----------------------------------------------------
	
    module->dump();

	//使用JIT引擎---------------------------------------
	//https://blog.csdn.net/xfxyy_sxfancy/article/details/50485090
	InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

	//创建ExecutionEngine
	initEE(std::unique_ptr<Module>(module));
	
	delete module ;
    return 0;
}
