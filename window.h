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
        std::unique_ptr<form> window;
        std::unique_ptr<place> layout;

        void makeLayout()
        {
            layout->div(
                "<weight=5>"
                "<vert weight=250<weight=5><weight=25<open weight=33%><save weight=33%><saveAs weight=33%>><input weight=25><button weight=25><output vert arrange=[25, repeated]><weight=5>>"
                "<weight=5>"
                "<vert<weight=5><text><weight=5>>"
                "<weight=5>");
        }

    public:
        Window() :
            window{ std::make_unique<form>(API::make_center(500, 310)) },
            layout{ std::make_unique<place>(*window) }
        {
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

        void Exec()
        {
            window->caption("Password generator");
            window->show();
            layout->collocate();
            exec();
        }
    };

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
            input{ std::make_unique<textbox>(window.Form()) },
            output{},
            generator{ std::make_unique<button>(window.Form()) }
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

    class FileManager
    {
    private:
        std::unique_ptr<button> saver;
        std::unique_ptr<button> saveAser;
        std::unique_ptr<button> opener;
        std::unique_ptr<textbox> text;

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
            path = getFile(true);
            if (!path.has_value())
                return;
            key = getPassword();
            if (!key.has_value())
                return;
            File f{ key.value() };
            f.Read(path.value());
            std::string txt{ f.Get() };
            text->select(true);
            text->del();
            text->append(txt, false);
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
            key = getPassword();
            if (!key.has_value())
                return;
            path = getFile(false);
            if (key.has_value() && path.has_value())
                saveAs(path.value(), key.value()); 
        }
        
        void saveAs(std::filesystem::path const &path, std::string const &key)
        {
            std::string txt{};
            std::size_t lines = text->text_line_count();
            for (std::size_t i = 0; i < lines; ++i)
            {
                auto temp = text->getline(i);
                if (temp.has_value())
                    txt.append(temp.value());
                txt.push_back('\n');
            }
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

        }

        void add()
        {
            window.Layout()["text"] << *text;
            window.Layout()["open"] << *opener;
            window.Layout()["save"] << *saver;
            window.Layout()["saveAs"] << *saveAser;
        }
    public:
        FileManager(Window& window, Pool& pool) :
            pool{ pool },
            window{ window },
            saver{ std::make_unique<button>(window.Form()) },
            saveAser{ std::make_unique<button>(window.Form()) },
            opener{ std::make_unique<button>(window.Form()) },
            text{ std::make_unique<textbox>(window.Form()) }
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