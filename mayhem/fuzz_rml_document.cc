// Fuzzes the whole document pipeline: the XML/RML parser, element instancing, inline <style> and
// RCSS, data bindings (a model named "fuzz" is available), layout and rendering, plus input events.
#include "rml_fuzz_common.h"

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	fuzz::Initialise();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	fuzz::Session session;
	Rml::ElementDocument* document = session.context->LoadDocumentFromMemory(Rml::String(reinterpret_cast<const char*>(data), size));
	if (!document)
		return -1;
	document->Show();
	session.Exercise();
	document->Close();
	session.context->Update();
	return 0;
}
