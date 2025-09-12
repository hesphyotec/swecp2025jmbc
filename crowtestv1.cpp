#include "crow.h"
#include <iostream>

int main(){

	crow::SimpleApp app;

	CROW_ROUTE(app, "/")([](){
		auto page = crow::mustache::load_text("testpage.html");
		return page;
	});

	CROW_ROUTE(app, "/<string>")([](std::string name){
		auto page = crow::mustache::load("testpage.html");
		crow::mustache::context ctx ({{"person", name}});
		return page.render(ctx);
	});

	app.port(18080).multithreaded().run();
	return 0;
}
