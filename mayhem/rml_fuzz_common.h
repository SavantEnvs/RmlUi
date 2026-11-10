// Shared setup for the RmlUi harnesses. The library is initialised once, with interfaces that do no
// file, clock or window access. Every input gets a fresh Context (and data model), so its result
// depends only on that input: the destructor also releases the font handles and glyph atlases an input created,
// so only the loaded font faces persist across inputs.
#pragma once

#include <RmlUi/Core.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <cstdint>
#include <cstdlib>

extern "C" const unsigned char rml_font_regular[], rml_font_regular_end[];
extern "C" const unsigned char rml_font_bold[], rml_font_bold_end[];

namespace fuzz {

inline double g_time = 0.0;

class System final : public Rml::SystemInterface {
public:
	double GetElapsedTime() override { return g_time; }
	bool LogMessage(Rml::Log::Type type, const Rml::String& message) override
	{
		// DataInterpreter::Error() logs this warning and then fires RMLUI_ERROR whenever an expression calls a
		// transform function or event callback that is unregistered or declines; that is a debug-build
		// diagnostic for the document author (a release build just returns false), so only that assert is let through.
		if (type == Rml::Log::LT_WARNING && message.compare(0, 22, "Error during execution") == 0)
		{
			expect_expression_error = true;
			return true;
		}
		if (type == Rml::Log::LT_ASSERT)
		{
			if (expect_expression_error)
			{
				expect_expression_error = false;
				return true;
			}
			abort();
		}
		return true;
	}

private:
	bool expect_expression_error = false;
};

// Opening a path would let a document read arbitrary files of the host, so no file ever opens.
class NoFiles final : public Rml::FileInterface {
public:
	Rml::FileHandle Open(const Rml::String&) override { return 0; }
	void Close(Rml::FileHandle) override {}
	size_t Read(void*, size_t, Rml::FileHandle) override { return 0; }
	bool Seek(Rml::FileHandle, long, int) override { return false; }
	size_t Tell(Rml::FileHandle) override { return 0; }
};

// Does not draw, but accepts every texture request with a fixed size so image layout is exercised.
class Render final : public Rml::RenderInterface {
public:
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override { return ++next; }
	void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override {}
	void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}
	Rml::TextureHandle LoadTexture(Rml::Vector2i& dimensions, const Rml::String&) override
	{
		dimensions = Rml::Vector2i(48, 32);
		return ++next;
	}
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override { return ++next; }
	void ReleaseTexture(Rml::TextureHandle) override {}
	void EnableScissorRegion(bool) override {}
	void SetScissorRegion(Rml::Rectanglei) override {}

private:
	uintptr_t next = 0;
};

inline void Initialise()
{
	// Never destroyed: RmlUi stays initialised until exit, and a render interface must not die before Shutdown.
	static System* const system = new System;
	static NoFiles* const files = new NoFiles;
	static Render* const render = new Render;
	Rml::SetSystemInterface(system);
	Rml::SetFileInterface(files);
	Rml::SetRenderInterface(render);
	if (!Rml::Initialise())
		abort();
	const Rml::Span<const Rml::byte> regular(rml_font_regular, rml_font_regular_end - rml_font_regular);
	const Rml::Span<const Rml::byte> bold(rml_font_bold, rml_font_bold_end - rml_font_bold);
	if (!Rml::LoadFontFace(regular, "LatoLatin", Rml::Style::FontStyle::Normal, Rml::Style::FontWeight::Normal) ||
		!Rml::LoadFontFace(bold, "LatoLatin", Rml::Style::FontStyle::Normal, Rml::Style::FontWeight::Bold))
		abort();
}

struct Item {
	Rml::String name = "item";
	int qty = 2;
};

// Values a data-bound document can read and write; recreated for every input.
struct Model {
	int i = 3;
	float f = 1.5f;
	bool b = true;
	Rml::String s = "hello";
	Rml::Vector<int> ints = {1, 2, 3};
	Rml::Vector<Rml::String> strs = {"a", "b", "c"};
	Item item;
	Rml::Vector<Item> items = {Item{"x", 1}, Item{"y", 2}};
};

class Session {
public:
	Session()
	{
		g_time = 0.0;
		context = Rml::CreateContext("fuzz", Rml::Vector2i(640, 480));
		if (!context)
			abort();
		Rml::DataModelConstructor constructor = context->CreateDataModel("fuzz");
		if (auto item_handle = constructor.RegisterStruct<Item>())
		{
			item_handle.RegisterMember("name", &Item::name);
			item_handle.RegisterMember("qty", &Item::qty);
		}
		constructor.RegisterArray<Rml::Vector<int>>();
		constructor.RegisterArray<Rml::Vector<Rml::String>>();
		constructor.RegisterArray<Rml::Vector<Item>>();
		constructor.Bind("i", &model.i);
		constructor.Bind("f", &model.f);
		constructor.Bind("b", &model.b);
		constructor.Bind("s", &model.s);
		constructor.BindFunc("colour", [](Rml::Variant& value) { value = Rml::String("#0a141e"); });
		constructor.Bind("ints", &model.ints);
		constructor.Bind("strs", &model.strs);
		constructor.Bind("item", &model.item);
		constructor.Bind("items", &model.items);
		constructor.BindFunc("fn", [](Rml::Variant& value) { value = 5; });
		constructor.BindEventCallback("ev", [](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) {});
		model_handle = constructor.GetModelHandle();
	}

	~Session()
	{
		Rml::RemoveContext("fuzz");
		Rml::Factory::ClearStyleSheetCache();
		Rml::Factory::ClearTemplateCache();
		Rml::ReleaseFontResources();
	}

	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;

	// Runs layout, rendering, animations and a fixed sequence of input events over the loaded documents.
	void Exercise()
	{
		g_time = 0.0;
		context->Update();
		context->Render();
		context->ProcessMouseMove(60, 40, 0);
		context->ProcessMouseButtonDown(0, 0);
		context->ProcessMouseButtonUp(0, 0);
		context->ProcessKeyDown(Rml::Input::KI_TAB, 0);
		context->ProcessTextInput("ab");
		context->ProcessKeyDown(Rml::Input::KI_BACK, 0);
		context->ProcessKeyDown(Rml::Input::KI_DOWN, 0);
		context->ProcessMouseWheel(Rml::Vector2f(0.f, 1.f), 0);
		g_time = 0.5;
		model_handle.DirtyAllVariables();
		context->Update();
		context->Render();
		context->SetDimensions(Rml::Vector2i(300, 200));
		context->SetDensityIndependentPixelRatio(2.f);
		g_time = 5.0;
		context->Update();
		context->Render();
	}

	Rml::Context* context = nullptr;
	Model model;
	Rml::DataModelHandle model_handle;
};

} // namespace fuzz
