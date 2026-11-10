// Fuzzes the data-binding expression parser and evaluator. The input is used verbatim, XML-escaped,
// as the expression in every kind of data attribute and in a {{ }} text binding below an element
// bound to the "fuzz" data model; a click then runs the event expressions.
#include "rml_fuzz_common.h"

namespace {
Rml::String EscapeXml(const uint8_t* data, size_t size)
{
	Rml::String out;
	for (size_t i = 0; i < size; i++)
	{
		switch (data[i])
		{
		case '&': out += "&amp;"; break;
		case '<': out += "&lt;"; break;
		case '"': out += "&quot;"; break;
		default: out += static_cast<char>(data[i]);
		}
	}
	return out;
}
}

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	fuzz::Initialise();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	if (size == 0)
		return -1;
	const Rml::String e = EscapeXml(data, size);
	const Rml::String rml = "<rml><head/><body><div id=\"m\" data-model=\"fuzz\" style=\"width: 300px; height: 200px\">"
							"<input type=\"text\" data-value=\"s\"/>"
							"<p data-if=\"" + e + "\">if</p>"
							"<p data-visible=\"" + e + "\">visible</p>"
							"<p data-attr-title=\"" + e + "\" data-attrif-lang=\"" + e + "\">attr</p>"
							"<p data-class-red=\"" + e + "\" data-style-width=\"" + e + "\">class</p>"
							"<p data-event-click=\"" + e + "\" data-event-mousedown=\"ev(" + e + ")\" style=\"width: 100%; height: 30px\">event</p>"
							"<div data-for=\"it : items\"><span>{{ it.name }}{{ " + e + " }}</span></div>"
							"<p>{{ " + e + " }}</p>"
							"</div></body></rml>";

	fuzz::Session session;
	Rml::ElementDocument* document = session.context->LoadDocumentFromMemory(rml);
	if (!document)
		return -1;
	document->Show();
	session.Exercise();
	document->Close();
	session.context->Update();
	return 0;
}
