extern "C" void print(const char*);

const char* module_name = "Test/Hello";
const char* module_version = "1.0";
const char* module_description = "Prints \"Hello, world\" text";
const char* module_requirements = "";
const char* module_developer = "PhoeniX";

void module()
{
	print("Hello, world!");
}
