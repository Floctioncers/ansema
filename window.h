#ifndef WINDOW_H
#define WINDOW_H

#include "thread_pool.h"
#include "chars_password.h"
#include "aes_transformator.h"
#include "parser.h"

#include <optional>
#include <functional>
#include <vector>
#include <unordered_map>
#include <nana/gui.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/filebox.hpp>
#include <nana/gui/msgbox.hpp>
#include <nana/system/dataexch.hpp>

namespace Window
{
    using Pool = ThreadPool::ThreadPool<std::function<void(void)>>;
    using Pass = CharsPassword::PasswordGenerator;
	using File = AesTransformator::AesFile;
    using namespace nana;
    class KeyPress
    {
    public:
        enum class Key : std::size_t { Invalid, Escape };
    private:
        static std::unordered_map<wchar_t, Key> map;
        Key key;
        bool shift;
        
        void setUp()
        {
            if (map.size() != 0)
                return;
            map['\x1b'] = Key::Escape;
        }
    public:
        KeyPress() : key{ Key::Invalid }, shift{ false } { setUp(); }
        KeyPress(KeyPress const&) = default;
        KeyPress(KeyPress&&) = default;
        KeyPress& operator=(KeyPress const&) = default;
        KeyPress& operator=(KeyPress&&) = default;
        ~KeyPress() = default;

        void setKey(Key key)
        {
            this->key = key;
        }

        void setKey(wchar_t c)
        {
            if (map.find(c) != map.cend())
            {
                key = map[c];
            }
        }
        void setShift(bool s)
        {
            shift = std::move(s);
        }

        std::size_t Hash() const
        {
            std::size_t times = 0;
            if (shift)
                ++times;
            std::hash<std::size_t> h{};
            return h(static_cast<std::size_t>(key)) << times;
        }

        bool Equals(KeyPress const& b) const
        {
            return (b.key == key) && (b.shift == shift);
        }
    };
    struct KeyPressHash
    {
        std::size_t operator()(KeyPress const& k) const
        {
            return k.Hash();
        }
    };
    bool operator==(KeyPress const& a, KeyPress const& b)
    {
        return a.Equals(b);
    }
    inline std::unordered_map<wchar_t, KeyPress::Key> KeyPress::map{};

    template<typename T, typename U>
    std::unique_ptr<T> GenerateChild(U const &parent)
    {
        std::unique_ptr<T> ptr{ std::make_unique<T>(parent) };
        T* view{ ptr.get() };
        view->events().key_char([view](nana::arg_keyboard const& keyboard)
        {
            nana::API::emit_event(nana::event_code::key_press, view->parent(), keyboard);
        });
        return std::move(ptr);
    }

    class Window
    {
    private:
		static nana::size const windowSize;
        std::unique_ptr<form> window;
        std::unique_ptr<place> layout;
        std::unordered_map<KeyPress, std::function<void(void)>, KeyPressHash> map;

        void makeLayout()
        {
            layout->div(
                "<weight=5>"
                "<vert weight=250"
					"<weight=5>"
					"<weight=25<open weight=33%><save weight=33%><saveAs weight=33%>>"
					"<input weight=25>"
					"<button weight=25>"
					"<output vert arrange=[25, repeated]>"
					"<weight=5>>"
                "<weight=5>"
                "<vert"
					"<weight=5>"
					"<weight=25<change weight=80>>"
					"<edit>"
					"<view>"
					"<weight=5>>"
                "<weight=5>");
        }
        void makeWindow()
        {
            window->events().key_press([this](nana::arg_keyboard const& keyboard)
            {
                KeyPress key{};
                key.setKey(keyboard.key);
                key.setShift(keyboard.shift);
                if (map.find(key) != map.cend())
                {
                    map[key]();
                }
            });
			nana::API::track_window_size(*window, windowSize, false);
        }

    public:
        Window() :
            window{ std::make_unique<form>(API::make_center(windowSize.width, windowSize.height)) },
            layout{ std::make_unique<place>(*window) },
            map{}
        {
            makeWindow();
            makeLayout();
        };

        Window(Window const&) = delete;
        Window(Window&&) = default;
        Window& operator=(Window const&) = delete;
        Window& operator=(Window&&) = default;
        ~Window() = default;

        form& Form()
        {
            return *window;
        }

        place& Layout()
        {
            return *layout;
        }

        void Register(KeyPress const &key, std::function<void(void)> &&fn)
        {
            map[key] = std::move(fn);
        }

        void Exec()
        {
            window->caption("Password generator");
            window->show();
            layout->collocate();
            exec();
        }
    };

	inline nana::size const Window::windowSize{ 600, 310 };

    class PasswordGenerator
    {
    private:
        const std::size_t outputSize;
        std::unique_ptr<textbox> input;
        std::vector<std::unique_ptr<textbox>> output;
        std::unique_ptr<button> generator;

        Pool &pool;
        Pass &pass;
        Window& window;

        void generate()
        {
            for (std::size_t i = 0; i < output.size(); ++i)
            {
                auto const in{ input->getline(0) };
                auto const out{ pass.Generate(std::move(in)) };
                output[i]->select(true);
                output[i]->del();
                output[i]->append(out.value_or("INVALID"), false);
            }
        }

        void copy(std::size_t pos)
        {
            output[pos]->select(true);
            output[pos]->copy();
            output[pos]->del();
            output[pos]->append("Copied!", false);
        }

        void makeInput()
        {
            input->caption("aaa-AAA-nnn-xxx-XXX");
            input->multi_lines(false);
        }

        void makeOutput()
        {
            for (std::size_t i = 0; i < outputSize; ++i)
            {
                output.push_back(std::make_unique<textbox>(window.Form()));
                output.back()->multi_lines(false);
                output.back()->editable(false);
                auto pos = output.size() - 1;
                output.back()->events().dbl_click([this, pos]() {
                    copy(pos);
                });
            }
        }

        void makeGenerator()
        {
            generator->caption("Generate!");
            generator->events().click([this]() {
                auto fn = [this]() { generate(); };
                pool.Append(std::move(fn));
            });
        }

        void add()
        {
            window.Layout()["input"] << *input;
            window.Layout()["button"] << *generator;
            for (auto &item : output)
            {
                window.Layout()["output"] << *item;
            }
        }

    public:
        PasswordGenerator(Window &window, Pass& pass, Pool& pool) :
            pool{ pool },
            pass{ pass },
            window{ window },
            outputSize{ 9 },
            input{ GenerateChild<textbox>(window.Form()) },
            output{},
            generator{ GenerateChild<button>(window.Form()) }
        {
            makeInput();
            makeOutput();
            makeGenerator();
            add();
        }

        PasswordGenerator(PasswordGenerator const&) = delete;
        PasswordGenerator(PasswordGenerator&&) = default;
        PasswordGenerator& operator=(PasswordGenerator const&) = delete;
        PasswordGenerator& operator=(PasswordGenerator&&) = default;
        ~PasswordGenerator() = default;
    };

	class TextManager
	{
	private:
		std::unique_ptr<textbox> edit;
		std::unique_ptr<textbox> view;
		std::unique_ptr<button> change;
		std::vector<std::vector<Parser::Block>> blocks;
		bool editting;
		static std::string const editCaption;
		static std::string const viewCaption;

		void transform()
		{
			auto fn = [this]()
			{
				blocks.clear();
				std::size_t count{ edit->text_line_count() };
				std::string replace{};
				for (std::size_t i = 0; i < count; ++i)
				{
					auto line{ edit->getline(i) };
					if (line.has_value())
					{
						std::string const& l{ line.value() };
						Parser::Parser p{ l };
						Parser::Transformator t{ l };
						std::string transformed{ t.Transform(p.GetTokens()) };
						replace.append(std::move(transformed));
						blocks.push_back(t.Get());
					}
					replace.push_back('\n');
				}
				view->reset(std::move(replace), true);
			};
			pool.Append(std::move(fn));
		}

        void copySecret()
        {
            auto pos = view->caret_pos();
            for (auto const& item : blocks[pos.y])
            {
                if ((item.first.first <= pos.x) &&
                    (item.first.second >= pos.x))
                {
                    nana::system::dataexch().set(item.second);
                    break;
                }
            }
        }

		void makeEdit()
		{

		}

		void makeView()
		{
			view->editable(false);
			view->bgcolor(nana::colors::light_grey);
			view->enable_caret();
            view->events().dbl_click([this]()
            {
                auto fn = [this]() { copySecret(); };
                pool.Append(std::move(fn));
            });
		}

		void modeSwitch()
		{
			editting = !editting;
			if (editting)
			{
				change->caption(viewCaption);
				window.Layout().field_display("edit", true);
				window.Layout().field_display("view", false);
				window.Layout().collocate();
			}
			else
			{
				transform();
				change->caption(editCaption);
				window.Layout().field_display("edit", false);
				window.Layout().field_display("view", true);
				window.Layout().collocate();
			}
		}

		void makeChange()
		{
			change->caption("Edit!");
			change->events().click([this]()
			{
				auto fn = [this]() { modeSwitch();  };
				pool.Append(std::move(fn));
			});
		}

		void show()
		{
			edit->show();
			view->show();
		}

		void hide()
		{
			edit->hide();
			view->hide();
		}

		void add()
		{
			window.Layout()["edit"] << *edit;
			window.Layout()["view"] << *view;
			window.Layout()["change"] << *change;
			window.Layout().field_display("edit", false);
			auto fn = [this]() { hide(); };
			auto gn = [this]() { show(); };
			KeyPress hideShortCut{};
			hideShortCut.setKey(KeyPress::Key::Escape);
			hideShortCut.setShift(false);
			KeyPress showShortCut{};
			showShortCut.setKey(KeyPress::Key::Escape);
			showShortCut.setShift(true);
			window.Register(hideShortCut, std::move(fn));
			window.Register(showShortCut, std::move(gn));
		}

		Window &window;
		Pool &pool;
	public:
		TextManager(Window& window, Pool& pool) :
			pool{ pool },
			window{ window },
			edit{ GenerateChild<textbox>(window.Form()) },
			view{ GenerateChild<textbox>(window.Form()) },
			change{ GenerateChild<button>(window.Form()) },
			editting{ false }, blocks{}
		{
			makeEdit();
			makeView();
			makeChange();
			add();
		}

		TextManager(TextManager const&) = delete;
		TextManager(TextManager&&) = default;
		TextManager& operator=(TextManager const&) = delete;
		TextManager& operator=(TextManager&&) = default;
		~TextManager() = default;

		std::string Get()
		{
			std::string txt{};
			std::size_t lines = edit->text_line_count();
			for (std::size_t i = 0; i < lines; ++i)
			{
				auto temp = edit->getline(i);
				if (temp.has_value())
					txt.append(temp.value());
				txt.push_back('\n');
			}
			return std::move(txt);
		}

		void Set(std::string &&txt)
		{
			edit->select(true);
			edit->del();
			edit->append(txt, false);
			transform();
		}
	};

	inline std::string const TextManager::editCaption{ "Edit!" };
	inline std::string const TextManager::viewCaption{ "View!" };

    class FileManager
    {
    private:
        std::unique_ptr<button> saver;
        std::unique_ptr<button> saveAser;
        std::unique_ptr<button> opener;
        std::unique_ptr<TextManager> text;

        std::optional<std::filesystem::path> path;
        std::optional<std::string> key;

        Window& window;
        Pool &pool;

        std::optional<std::filesystem::path> getFile(bool open)
        {
            filebox box{ window.Form(), open };
            box.allow_multi_select(false);
            box.add_filter("Secret file", "*.scrt");
            box.init_path(".");
            box.init_file("my_secret.scrt");
            auto out = box.show();
            if (out.size() == 0)
                return std::nullopt;
            else
                return std::make_optional<std::filesystem::path>(out.back());
        }

        std::optional<std::string> getPassword()
        {
            inputbox input{ window.Form(), "Please input your password.", "Password" };
            inputbox::text password{ "Password:" };
            password.mask_character('*');
            if (input.show_modal(password))
                return std::make_optional<std::string>(password.value());
            return std::nullopt;
        }

        void open()
        {
            auto tempPath = getFile(true);
            if (!tempPath.has_value())
                return;
            auto tempKey = getPassword();
            if (!tempKey.has_value())
                return;
			key = std::move(tempKey);
			path = std::move(tempPath);
            File f{ key.value() };
            f.Read(path.value());
            std::string txt{ f.Get() };
			text->Set(std::move(txt));
        }

        void save()
        {
            if (!key.has_value())
                key = getPassword();
            if (!key.has_value())
                return;
            if (!path.has_value())
                path = getFile(false);
            if (key.has_value() && path.has_value())
                saveAs(path.value(), key.value());
        }

        void saveAs()
        {
            auto tempKey = getPassword();
            if (!tempKey.has_value())
                return;
            auto tempPath = getFile(false);
			if (tempKey.has_value() && tempPath.has_value())
			{
				key = std::move(tempKey);
				path = std::move(tempPath);
				saveAs(path.value(), key.value());
			}
        }
        
        void saveAs(std::filesystem::path const &path, std::string const &key)
        {
			std::string txt{ text->Get() };
            File f{ key };
            f.Append(std::move(txt));
            f.Write(path);
        }

        void makeSaver()
        {
            saver->caption("Save file!");
            saver->events().click([this]() {
                auto fn = [this]() { save(); };
                pool.Append(std::move(fn));
            });
        }

        void makeSaveAser()
        {
            saveAser->caption("Save file as..!");
            saveAser->events().click([this]() {
                auto fn = [this]() { saveAs(); };
                pool.Append(std::move(fn));
            });
        }

        void makeOpener()
        {
            opener->caption("Open file!");
            opener->events().click([this]() {
                auto fn = [this]() { open(); };
                pool.Append(std::move(fn));
            });
        }

        void makeText()
        {
			//TODO
        }

        void add()
        {
            window.Layout()["open"] << *opener;
            window.Layout()["save"] << *saver;
            window.Layout()["saveAs"] << *saveAser;
        }
    public:
        FileManager(Window& window, Pool& pool) :
            pool{ pool },
            window{ window },
            saver{ GenerateChild<button>(window.Form()) },
            saveAser{ GenerateChild<button>(window.Form()) },
            opener{ GenerateChild<button>(window.Form()) },
            text{ std::make_unique<TextManager>(window, pool) }
        {
            makeSaver();
            makeSaveAser();
            makeOpener();
            makeText();
            add();
        }

        FileManager(FileManager const&) = delete;
        FileManager(FileManager&&) = default;
        FileManager& operator=(FileManager const&) = delete;
        FileManager& operator=(FileManager&&) = default;
        ~FileManager() = default;
    };
}
#endif