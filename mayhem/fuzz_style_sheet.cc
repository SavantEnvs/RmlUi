// Fuzzes RCSS: the style sheet parser (rules, selectors, @media, @keyframes, decorators, property
// values) and then the cascade, selector matching, layout and animations on a fixed document.
#include "rml_fuzz_common.h"

namespace {
const char fixed_document[] = R"(<rml>
<head><title>t</title></head>
<body id="body" class="a b">
<div id="d1" class="box a"><p class="x">Hello <span id="s1">world</span> <em>text</em> <b>bold</b></p></div>
<div id="d2" class="box b" style="display: flex"><div class="item">1</div><div class="item">2</div><div class="item">3</div></div>
<div id="d3" class="a" style="overflow: auto; height: 40px"><p>one</p><p>two</p><p>three</p><p>four</p></div>
<img id="img" src="a.png"/>
<input type="text" id="in" value="abc"/>
<input type="checkbox" id="cb"/>
<input type="range" id="rg" min="0" max="10"/>
<select id="sel"><option value="1">One</option><option value="2" selected>Two</option></select>
<textarea id="ta">text area</textarea>
<button id="btn">Button</button>
<table><tr><td>a</td><td>b</td></tr></table>
<ul><li>x</li><li class="last">y</li></ul>
<handle id="hd" size="10px 10px"/>
</body>
</rml>)";
}

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
	fuzz::Initialise();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	Rml::SharedPtr<Rml::StyleSheetContainer> style_sheet =
		Rml::Factory::InstanceStyleSheetString(Rml::String(reinterpret_cast<const char*>(data), size));
	if (!style_sheet)
		return -1;

	fuzz::Session session;
	Rml::ElementDocument* document = session.context->LoadDocumentFromMemory(fixed_document);
	if (!document)
		abort();
	document->SetStyleSheetContainer(std::move(style_sheet));
	document->Show();
	session.Exercise();
	document->Close();
	session.context->Update();
	return 0;
}
