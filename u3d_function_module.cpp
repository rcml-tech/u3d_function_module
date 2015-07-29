/*
* File: messages.cpp
* Author: m79lol, iskinmike
*
*/

#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS 
#endif

#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include <SimpleIni.h>
#include "module.h"
#include "function_module.h"
#include "u3d_function_module.h"
#include "messages_functions.h"

// GLOBAL VARIABLES
const unsigned int COUNT_U3D_FUNCTIONS = 13;
extern std::string getConfigPath();
bool is_fail_to_prepare;

void u3dFunctionModule::connect_handler(const boost::system::error_code& ec)
{
	if (ec) {
		colorPrintf(ConsoleColor(ConsoleColor::red), "can't connect to socket, error code: %d\n", ec.value());
		is_fail_to_prepare = true;
		robot_io_service_.stop();
	}
	else {
		colorPrintf(ConsoleColor(ConsoleColor::green), "connect to socket\n");
	}
}

void u3dFunctionModule::write_handler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	if (ec) {
		colorPrintf(ConsoleColor(ConsoleColor::red), "can't send to socket, error code: %d\n", ec.value());
		is_fail_to_prepare = true;
		robot_io_service_.stop();
		throw std::exception();
	}
}

void u3dFunctionModule::read_handler(SocketAndBuffer *sock_and_buff_struct, const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	if (sock_and_buff_struct->buffer_){
		char perc = '%';
		char amper = '&';
		std::string temp_message(sock_and_buff_struct->buffer_, bytes_transferred);

		while (temp_message.find(amper) != std::string::npos) {
			unsigned int PosAmper = temp_message.find(amper);
			unsigned int PosPerc = temp_message.find(perc);

			std::string strToProcess = temp_message.substr(PosPerc, PosAmper - PosPerc + 1);
			temp_message.assign(temp_message.substr(PosAmper + 1));

			int uniq_id = extractUniq_Id(strToProcess);
			postmans_map_of_mailed_messages[uniq_id]->string_var = strToProcess;
			postmans_map_of_mailed_messages[uniq_id]->bool_var = true;
			postmans_map_of_mailed_messages[uniq_id]->cond_var->notify_one();
			postmans_map_of_mailed_messages.erase(uniq_id);
		};
	}
	(*sock_and_buff_struct->socket_).async_receive(
			boost::asio::buffer(sock_and_buff_struct->buffer_, 1024),
			boost::bind(&u3dFunctionModule::read_handler, this, sock_and_buff_struct, _1, _2)
		);
}

u3dFunctionModule::u3dFunctionModule() : module_socket(robot_io_service_), postman_thread_waker_flag(false){
	u3d_functions = new FunctionData*[COUNT_U3D_FUNCTIONS];
	system_value function_id = 0;

	FunctionData::ParamTypes *Params = new FunctionData::ParamTypes[3];
	Params[0] = FunctionData::ParamTypes::FLOAT;
	Params[1] = FunctionData::ParamTypes::FLOAT;
	Params[2] = FunctionData::ParamTypes::FLOAT;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 3, Params, "createWorld");
	function_id++;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 0, NULL, "destroyWorld");
	function_id++;

	Params = new FunctionData::ParamTypes[1];
	Params[0] = FunctionData::ParamTypes::FLOAT;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 1, Params, "deleteObject");
	function_id++;

	Params = new FunctionData::ParamTypes[9];
	Params[0] = FunctionData::ParamTypes::FLOAT;
	Params[1] = FunctionData::ParamTypes::FLOAT;
	Params[2] = FunctionData::ParamTypes::FLOAT;
	Params[3] = FunctionData::ParamTypes::FLOAT;
	Params[4] = FunctionData::ParamTypes::FLOAT;
	Params[5] = FunctionData::ParamTypes::FLOAT;
	Params[6] = FunctionData::ParamTypes::FLOAT;
	Params[7] = FunctionData::ParamTypes::FLOAT;
	Params[8] = FunctionData::ParamTypes::STRING;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 9, Params, "createCube");
	function_id++;

	Params = new FunctionData::ParamTypes[6];
	Params[0] = FunctionData::ParamTypes::FLOAT;
	Params[1] = FunctionData::ParamTypes::FLOAT;
	Params[2] = FunctionData::ParamTypes::FLOAT;
	Params[3] = FunctionData::ParamTypes::FLOAT;
	Params[4] = FunctionData::ParamTypes::FLOAT;
	Params[5] = FunctionData::ParamTypes::STRING;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 6, Params, "createSphere");
	function_id++;

	Params = new FunctionData::ParamTypes[10];
	Params[0] = FunctionData::ParamTypes::FLOAT;
	Params[1] = FunctionData::ParamTypes::FLOAT;
	Params[2] = FunctionData::ParamTypes::FLOAT;
	Params[3] = FunctionData::ParamTypes::FLOAT;
	Params[4] = FunctionData::ParamTypes::FLOAT;
	Params[5] = FunctionData::ParamTypes::FLOAT;
	Params[6] = FunctionData::ParamTypes::FLOAT;
	Params[7] = FunctionData::ParamTypes::FLOAT;
	Params[8] = FunctionData::ParamTypes::STRING;
	Params[9] = FunctionData::ParamTypes::STRING;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 10, Params, "createModel");
	function_id++;

	Params = new FunctionData::ParamTypes[2];
	Params[0] = FunctionData::ParamTypes::FLOAT;
	Params[1] = FunctionData::ParamTypes::STRING;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 2, Params, "changeColor");
	function_id++;

	Params = new FunctionData::ParamTypes[7];
	Params[0] = FunctionData::ParamTypes::FLOAT;
	Params[1] = FunctionData::ParamTypes::FLOAT;
	Params[2] = FunctionData::ParamTypes::FLOAT;
	Params[3] = FunctionData::ParamTypes::FLOAT;
	Params[4] = FunctionData::ParamTypes::FLOAT;
	Params[5] = FunctionData::ParamTypes::FLOAT;
	Params[6] = FunctionData::ParamTypes::FLOAT;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 7, Params, "moveObject");
	function_id++;

	Params = new FunctionData::ParamTypes[2];
	Params[0] = FunctionData::ParamTypes::FLOAT;
	Params[1] = FunctionData::ParamTypes::FLOAT;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 2, Params, "changeStatus");
	function_id++;

	Params = new FunctionData::ParamTypes[1];
	Params[0] = FunctionData::ParamTypes::FLOAT;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 1, Params, "getX");
	function_id++;

	Params = new FunctionData::ParamTypes[1];
	Params[0] = FunctionData::ParamTypes::FLOAT;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 1, Params, "getY");
	function_id++;

	Params = new FunctionData::ParamTypes[1];
	Params[0] = FunctionData::ParamTypes::FLOAT;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 1, Params, "getZ");
	function_id++;

	Params = new FunctionData::ParamTypes[1];
	Params[0] = FunctionData::ParamTypes::FLOAT;

	u3d_functions[function_id] = new FunctionData(function_id + 1, 1, Params, "getAngle");
	function_id++;
};

FunctionResult* u3dFunctionModule::executeFunction(system_value function_index, void **args) {
	if ((function_index < 1) || (function_index > (int) COUNT_U3D_FUNCTIONS) ) {
		return NULL;
	}
	
	try {
		// new test if we don't connect to socket or have problems with connection
		if (is_fail_to_prepare) { throw std::exception(); }
		variable_value rez = 0;
		switch (function_index) {
		case 1:{
			variable_value *input1 = (variable_value *)args[0];
			variable_value *input2 = (variable_value *)args[1];
			variable_value *input3 = (variable_value *)args[2];
			createWorld((int)*input1, (int)*input2, (int)*input3);
			break;
		}
		case 2:{
			destroyWorld();
			break;
		}
		case 3:{
			variable_value *input1 = (variable_value *)args[0];
			deleteObject((int)*input1);
			break;
		}
		case 4:{
			variable_value *input1 = (variable_value *)args[0];
			variable_value *input2 = (variable_value *)args[1];
			variable_value *input3 = (variable_value *)args[2];
			variable_value *input4 = (variable_value *)args[3];
			variable_value *input5 = (variable_value *)args[4];
			variable_value *input6 = (variable_value *)args[5];
			variable_value *input7 = (variable_value *)args[6];
			variable_value *input8 = (variable_value *)args[7];
			std::string input9((const char *)args[8]);
			rez = createCube((int)*input1, (int)*input2, (int)*input3, (int)*input4, (int)*input5, (int)*input6, (int)*input7, (int)*input8, input9);
			break;
		}
		case 5:{
			variable_value *input1 = (variable_value *)args[0];
			variable_value *input2 = (variable_value *)args[1];
			variable_value *input3 = (variable_value *)args[2];
			variable_value *input4 = (variable_value *)args[3];
			variable_value *input5 = (variable_value *)args[4];
			std::string input6((const char *)args[5]);
			rez = createSphere((int)*input1, (int)*input2, (int)*input3, (int)*input4, (int)*input5, input6);
			break;
		}
		case 6:{
			variable_value *input1 = (variable_value *)args[0];
			variable_value *input2 = (variable_value *)args[1];
			variable_value *input3 = (variable_value *)args[2];
			variable_value *input4 = (variable_value *)args[3];
			variable_value *input5 = (variable_value *)args[4];
			variable_value *input6 = (variable_value *)args[5];
			variable_value *input7 = (variable_value *)args[6];
			variable_value *input8 = (variable_value *)args[7];
			std::string input9((const char *)args[8]);
			std::string input10((const char *)args[9]);
			rez = createModel((int)*input1, (int)*input2, (int)*input3, (int)*input4, (int)*input5, (int)*input6, (int)*input7, (int)*input8, input9, input10);
			break;
		}
		case 7:{
			variable_value *input1 = (variable_value *)args[0];
			std::string input2((const char *)args[1]);
			changeColor((int)*input1, input2);
			break;
		}
		case 8:{
			variable_value *input1 = (variable_value *)args[0];
			variable_value *input2 = (variable_value *)args[1];
			variable_value *input3 = (variable_value *)args[2];
			variable_value *input4 = (variable_value *)args[3];
			variable_value *input5 = (variable_value *)args[4];
			variable_value *input6 = (variable_value *)args[5];
			variable_value *input7 = (variable_value *)args[6];
			moveObject((int)*input1, (int)*input2, (int)*input3, (int)*input4, (int)*input5, (int)*input6, (int)*input7);
			break;
		}
		case 9:{
			variable_value *input1 = (variable_value *)args[0];
			variable_value *input2 = (variable_value *)args[1];
			changeStatus((int)*input1, (int)*input2);
			break;
		}
		case 10:{
			variable_value *input1 = (variable_value *)args[0];
			rez = getX((int)*input1);
			break;
		}
		case 11:{
			variable_value *input1 = (variable_value *)args[0];
			rez = getY((int)*input1);
			break;
		}
		case 12:{
			variable_value *input1 = (variable_value *)args[0];
			rez = getZ((int)*input1);
			break;
		}
		case 13:{
			variable_value *input1 = (variable_value *)args[0];
			rez = getAngle((int)*input1);
			break;
		}
		};
		return new FunctionResult(1, rez);
	}
	catch (...){
		return new FunctionResult(0);
	};
};

FunctionData** u3dFunctionModule::getFunctions(unsigned int *count_functions) {
	*count_functions = COUNT_U3D_FUNCTIONS;
	return u3d_functions;
};
const char* u3dFunctionModule::getUID() {
	return "u3dfunction_dll";
};
void *u3dFunctionModule::writePC(unsigned int *buffer_length) {
	*buffer_length = 0;
	return NULL;
}
void u3dFunctionModule::readPC(void *buffer, unsigned int buffer_length) {
}
int u3dFunctionModule::startProgram(int uniq_index) {
	if (is_fail_to_prepare){
		return 1;
	}
	return 0;
}
int u3dFunctionModule::endProgram(int uniq_index) {
	return 0;
}
void u3dFunctionModule::destroy() {
	robot_io_service_.stop();

	for (unsigned int j = 0; j < COUNT_U3D_FUNCTIONS; ++j) {
		if (u3d_functions[j]->count_params) {
			delete[] u3d_functions[j]->params;
		}
		delete u3d_functions[j];
	}
	delete[] u3d_functions;

	postman_exit_mutex.lock();
	postman_thread_exit = true;
	postman_exit_mutex.unlock();

	module_mutex.lock();
	postman_thread_waker_flag = true;
	postman_thread_waker.notify_one();
	module_mutex.unlock();
	// wait to close thread;
	module_postman_thread->join();
	module_reciever_thread->join();
	delete module_postman_thread;
	delete module_reciever_thread;
	delete box_of_messages;
	delete data_for_shared_memory;
	delete sock_and_buff_struct;
	boost::interprocess::shared_memory_object::remove("PostmansSharedMemory");
	delete this;
};

void u3dFunctionModule::prepare(colorPrintfModule_t *colorPrintf_p, colorPrintfModuleVA_t *colorPrintfVA_p){
	this->colorPrintf_p = colorPrintfVA_p;
	is_fail_to_prepare = false;
	postman_thread_exit = false;
	std::string ConfigPath = "";
#ifdef _WIN32
	ConfigPath = getConfigPath();
#else
	Dl_info PathToSharedObject;
	void * pointer = reinterpret_cast<void*> (getFunctionModuleObject);
	dladdr(pointer, &PathToSharedObject);
	std::string dltemp(PathToSharedObject.dli_fname);

	int dlfound = dltemp.find_last_of("/");

	dltemp = dltemp.substr(0, dlfound);
	dltemp += "/config.ini";

	ConfigPath.append(dltemp.c_str());
#endif

	CSimpleIniA ini;
	ini.SetMultiKey(true);

	if (ini.LoadFile(ConfigPath.c_str()) < 0) {
		colorPrintf(ConsoleColor(ConsoleColor::red), "Can't load '%s' file!\n", ConfigPath.c_str());
		is_fail_to_prepare = true;
		return;
	}
	int port;
	std::string IP;
	port = ini.GetLongValue("connection", "port", 0);
	if (!port) {
		colorPrintf(ConsoleColor(ConsoleColor::red), "Port is empty\n");
		is_fail_to_prepare = true;
		return;
	}
	IP = ini.GetValue("connection", "ip", "");
	if (IP == ""){
		colorPrintf(ConsoleColor(ConsoleColor::red), "IP is empty\n");
		is_fail_to_prepare = true;
		return;
	}

	// Create and connect to socket
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(IP.c_str()), port);
	module_socket.async_connect(endpoint, boost::bind(&u3dFunctionModule::connect_handler, this, _1));
	
	box_of_messages = new std::vector<CondBoolString *>();

	// Create File Mapping
	boost::interprocess::shared_memory_object shm_obj(
		boost::interprocess::open_or_create,
		"PostmansSharedMemory",
		boost::interprocess::read_write
	);

	shm_obj.truncate(sizeof(MutexAndBoxVector*));
	boost::interprocess::mapped_region region(shm_obj, boost::interprocess::read_write);
	data_for_shared_memory = new MutexAndBoxVector();

	data_for_shared_memory->box = box_of_messages;
	data_for_shared_memory->mtx = &module_mutex;
	data_for_shared_memory->cond_postman_thread_waker = &postman_thread_waker;
	data_for_shared_memory->bool_postman_thread_waker_flag = &postman_thread_waker_flag;

	std::memcpy(region.get_address(), &data_for_shared_memory, region.get_size());
	// Create postman thread
	module_postman_thread = new boost::thread(boost::bind(&u3dFunctionModule::modulePostmanThread,this));
};

void u3dFunctionModule::colorPrintf(ConsoleColor colors, const char *mask, ...) {
	va_list args;
	va_start(args, mask);
	(*colorPrintf_p)(this, colors, mask, args);
	va_end(args);
}

void u3dFunctionModule::modulePostmanThread(){
	// create reciever thread
	module_reciever_thread = new boost::thread(boost::bind(&u3dFunctionModule::moduleRecieverThread,this));

	std::string mailed_message("");

	int postmans_uniq_id = 0;

	while (true){
		// check to exit
		postman_exit_mutex.lock();
		if (postman_thread_exit){ break; }
		postman_exit_mutex.unlock();
		// wait for new message in box
		boost::unique_lock<boost::mutex> lock(postman_thread_mutex);
		while (!postman_thread_waker_flag)
		{
			postman_thread_waker.wait(lock);
		}
		module_mutex.lock();
		postman_thread_waker_flag = false;
		module_mutex.unlock();
		// check box with messages
		module_mutex.lock();
		for (auto i = (*box_of_messages).begin(); i != (*box_of_messages).end(); i++){
			postmans_map_of_mailed_messages[postmans_uniq_id] = (*i);
			postmans_uniq_id++;
		}
		(*box_of_messages).clear();
		module_mutex.unlock();
		for (auto i = postmans_map_of_mailed_messages.begin(); i != postmans_map_of_mailed_messages.end(); i++){
			if (i->second->string_var != ""){
				mailed_message = "%%" + returnStr(i->first) + "+" + i->second->string_var + "&";  // construct message
				// send to socket
				module_socket.async_send(
					boost::asio::buffer(mailed_message.c_str(), mailed_message.length()),
					boost::bind(&u3dFunctionModule::write_handler, this, _1, _2)
				);
				i->second->string_var.assign("");
			}
		}
	}
};

void u3dFunctionModule::moduleRecieverThread(){
	char recv_message[1024];
	sock_and_buff_struct = new SocketAndBuffer();
	sock_and_buff_struct->buffer_ = recv_message;
	sock_and_buff_struct->socket_ = &module_socket;

	module_socket.async_receive(
		boost::asio::buffer(recv_message),
		boost::bind( &u3dFunctionModule::read_handler, this, sock_and_buff_struct, _1, _2)
	);
	robot_io_service_.run();
}

PREFIX_FUNC_DLL FunctionModule* getFunctionModuleObject() {
	return new u3dFunctionModule();
};
