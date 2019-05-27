#ifndef WINDOW_H
#define WINDOW_H

#include "thread_pool.h"
#include "chars_password.h"
#include "aes_transformator.h"

#include <optional>
#include <functional>
#include <vector>
#include <nana/gui.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/filebox.hpp>
#include <nana/gui/msgbox.hpp>

namespace Window
{
    using Pool = ThreadPool::ThreadPool<std::function<void(void)>>;
    using Pass = CharsPassword::PasswordGenerator;
	using File = AesTransformator::AesFile;
    using namespace nana;
    class Window
    {
    private:
        std::size_t const outputSize;

        std::unique_ptr<form> window;
        std::unique_ptr<textbox> input;
        std::vector<std::unique_ptr<textbox>> output;
        std::unique_ptr<button> generateButton;
        std::unique_ptr<button> saveButton;
        std::unique_ptr<button> openButton;
        std::unique_ptr<place> layout;
        std::unique_ptr<textbox> text;

        Pass &pass;
        Pool &pool;

		void save()
		{
			std::string txt{};
			std::size_t lines = text->text_line_count();
			for (std::size_t i = 0; i < lines; ++i)
			{
				auto temp = text->getline(i);
				if (temp.has_value())
					txt.append(temp.value());
			}
			//TODO read password from user!
			File f{"simplekey"};
			f.Append(std::move(txt));
			//TODO read file location from user!
			f.Write("./secret.scrt");
		}

		void open()
		{
			//TODO read password from user!
			File f{ "simplekey" };
			//TODO read file location from user!
			f.Read("./secret.scrt");
			std::string txt{ f.Get() };
			text->select(true);
			text->del();
			text->append(txt, false);
		}

        void generate(std::size_t i)
        {
            auto const in{ input->getline(0) };
            auto const out{ pass.Generate(std::move(in)) };
            output[i]->select(true);
            output[i]->del();
            output[i]->append(out.value_or("INVALID"), false);
        }

        void generate()
        {
            for (std::size_t i = 0; i < outputSize; ++i)
            {
                generate(i);
            }
        }

        void copy(textbox &text)
        {
            text.select(true);
            text.copy();
            text.del();
            text.append("Copied!", false);
        }

        std::unique_ptr<textbox> makeInput()
        {
            std::unique_ptr<textbox> input{ std::make_unique<textbox>(*window) };
            input->caption("aaa-AAA-nnn-xxx-XXX");
            input->multi_lines(false);
            return input;
        }

        std::unique_ptr<button> makeSaveButton()
        {
            std::unique_ptr<button> btn{ std::make_unique<button>(*window) };
            btn->caption("Save file!");
            btn->events().click([this]() {
				auto fn = [this]() { save(); };
                pool.Append(std::move(fn));
            });
            return btn;
        }

        std::unique_ptr<button> makeOpenButton()
        {
            std::unique_ptr<button> btn{ std::make_unique<button>(*window) };
            btn->caption("Open file!");
            btn->events().click([this]() {
				auto fn = [this]() { open(); };
                pool.Append(std::move(fn));
            });
            return btn;
        }

        std::unique_ptr<button> makeGenerateButton()
        {
            std::unique_ptr<button> btn{ std::make_unique<button>(*window) };
            btn->caption("Generate!");
            btn->events().click([this]() {
                auto fn = [this]() { generate(); };
                pool.Append(std::move(fn));
            });
            return btn;
        }

        std::unique_ptr<textbox> makeText()
        {
            std::unique_ptr<textbox> text{ std::make_unique<textbox>(*window) };
            return text;
        }

        std::unique_ptr<textbox> makeOutput()
        {
            std::unique_ptr<textbox> output{ std::make_unique<textbox>(*window) };
            output->multi_lines(false);
            output->editable(false);
            auto text = output.get();
            output->events().dbl_click([this, text]() {
                copy(*text);
            });
            return output;
        }

        std::unique_ptr<place> makeLayout()
        {
            std::unique_ptr<place> layout{ std::make_unique<place>(*window) };

            input = makeInput();
            generateButton = makeGenerateButton();
            output.clear();
            for (std::size_t i = 0; i < outputSize; ++i)
            {
                output.push_back(makeOutput());
            }
            text = makeText();
            saveButton = makeSaveButton();
            openButton = makeOpenButton();

            layout->div(
                "<weight=5>"
                "<vert weight=250<weight=5><weight=25<open weight=50%><save weight=50%>><input weight=25><button weight=25><output vert arrange=[25, repeated]><weight=5>>"
                "<weight=5>"
                "<vert<weight=5><text><weight=5>>"
                "<weight=5>");
            (*layout)["input"] << *input;
            (*layout)["button"] << *generateButton;
            for (auto &item : output)
            {
                (*layout)["output"] << *item;
            }
            (*layout)["text"] << *text;
            (*layout)["open"] << *openButton;
            (*layout)["save"] << *saveButton;
            layout->collocate();
            return layout;
        }

    public:
        Window(Pass &pass, Pool &pool) :
            outputSize{ 9 }, window
            { std::make_unique<form>(API::make_center(500, 310)) },
            input{ nullptr }, output{ outputSize },
            generateButton{ nullptr }, layout{ nullptr },
            text{nullptr}, pass{ pass }, pool{ pool } {};

        Window(Window const&) = delete;
        Window(Window&&) = default;
        Window& operator=(Window const&) = delete;
        Window& operator=(Window&&) = default;
        ~Window() = default;

        void Exec()
        {
            window->caption("Password generator");
            window->show();
            layout = makeLayout();
            exec();
        }
    };
}
#endif