#include "window.h"
#include "chars_password.h"
#include "thread_pool.h"
#include "aes_transformator.h"

int main()
{
    CharsPassword::PasswordGenerator pass{};
    ThreadPool::ThreadPool<std::function<void(void)>> pool{ 2 };
    Window::Window w{};
    Window::PasswordGenerator generator{ w, pass, pool };
    Window::FileManager manager{w, pool };
    pool.Start();
    w.Exec();
}