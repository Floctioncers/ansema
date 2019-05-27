#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <vector>
#include <atomic>
#include <filesystem>
#include <fstream>

namespace ThreadPool
{
    template<typename T>
    class Node
    {
    private:
        std::atomic<T*> value;
        std::atomic<Node<T>*> next;

        Node* Last()
        {
            Node* curr{ next.load() };
            Node* last{ this };
            while (curr != nullptr)
            {
                last = curr;
                curr = last->next.load();
            }
            return last;
        }
    public:
        Node(T* val) : value{ val }, next{ nullptr } {}
        Node(Node<T> const&) = delete;
        Node(Node<T>&&) = delete;
        Node<T>& operator=(Node<T> const&) = delete;
        Node<T>& operator=(Node<T>&&) = delete;
        ~Node()
        {
            T* val{ value.exchange(nullptr) };
            if (val != nullptr)
                delete val;
            Node* node{ next.exchange(nullptr) };
            if (node != nullptr)
                delete node;
        }

        void Append(Node<T> *item)
        {
            Node<T> *last{ nullptr };
            Node<T> *null{ nullptr };
            std::atomic<Node<T>*> test{ nullptr };
            do
            {
                last = Last();
                test.store(last->next.load());
            } while (!test.compare_exchange_weak(null, item));
        }

        Node<T> *Tail()
        {
            Node<T> *tail{ next.exchange(nullptr) };
            return tail;
        }

        T* Value()
        {
            T* val{ value.exchange(nullptr) };
            return val;
        }
    };

    template<typename T>
    class Queue
    {
    private:
        std::atomic<Node<T>*> head;
        std::atomic<std::uint64_t> used;

        void AppendNode(Node<T>* appended)
        {
            ++used;
            Node<T> *node{ head.load() };
            Node<T> *null{ nullptr };
            if (!head.compare_exchange_strong(null, appended))
            {
                node->Append(appended);
            }
            --used;
        }

    public:
        Queue() = default;
        Queue(Queue const&) = delete;
        Queue(Queue&&) = delete;
        Queue& operator=(Queue const&) = delete;
        Queue& operator=(Queue&&) = delete;
        ~Queue()
        {
            Node<T> *node{ head.exchange(nullptr) };
            if (node != nullptr)
                delete node;
        }

        template<typename... Args>
        void Append(Args&&... args)
        {
            Node<T> *appended{ new Node<T>{ new T{std::forward<Args>(args)...} } };
            AppendNode(appended);
        }

        std::unique_ptr<T> Pop()
        {
            Node<T> *node{ head.load() };
            head.store(node->Tail());
            T* val{ node->Value() };
            auto wait{ used.load() };

            while (wait > 0) {
                std::this_thread::yield();
                wait = used.load();
            }

            Node<T> *current{ head.load() };
            AppendNode(node->Tail());

            if (current != nullptr)
                delete node;
            return std::unique_ptr<T>{ val };
        }

        bool Empty()
        {
            Node<T> *node{ head.load() };
            return node == nullptr;
        }
    };

    template<typename T>
    class ThreadPool
    {
    private:
        std::vector<std::thread> threads;
        Queue<T> commands;
        std::atomic<bool> run;

        void Process()
        {
            bool check{ run.load() };
            while (check)
            {
                if (commands.Empty())
                {
                    std::this_thread::yield();
                }
                else
                {
                    auto fn{ commands.Pop() };
                    if (fn != nullptr)
                        (*fn)();
                }
                check = run.load();
            }
        }
    public:
        ThreadPool(std::size_t threads) : threads{ threads }, commands{}, run{ false } {}
        ThreadPool(ThreadPool const&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool const&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;
        ~ThreadPool()
        {
            for (std::size_t i = 0; i < threads.size(); ++i)
            {
                run.store(false);
                if (threads[i].joinable())
                {
                    threads[i].join();
                }
            }
        }

        void Stop()
        {
            run.store(false);
        }

        bool IsRunning()
        {
            return run.load();
        }

        void Start()
        {
            run.store(true);
            for (std::size_t i = 0; i < threads.size(); ++i)
            {
                threads[i] = std::thread{ [&]() { Process(); } };
            }
        }

        void Append(T&& fn)
        {
            commands.Append(std::move(fn));
        }

        void Append(T const &fn)
        {
            commands.Append(fn);
        }
    };



    template<typename T>
    class Message
    {
    protected:
        std::filesystem::path path;

    public:
        Message(std::filesystem::path&& path) : path { std::move(path) } {}
        Message(Message const&) = delete;
        Message(Message&&) = delete;
        Message& operator=(Message const&) = delete;
        Message& operator=(Message&&) = delete;
        virtual ~Message() = default;

        virtual void operator()() = 0;
    };

    template<typename T>
    class WriteMessage : public Message<T>
    {
    private:
        std::vector<T> msg;
        
    public:
        WriteMessage(std::vector<T> &&message, std::filesystem::path &&path) : Message<T>{ std::move(path) }, msg{ std::move(msg) } {}
        WriteMessage(WriteMessage const&) = delete;
        WriteMessage(WriteMessage&&) = default;
        WriteMessage& operator=(WriteMessage const&) = delete;
        WriteMessage& operator=(WriteMessage&&) = default;
        ~WriteMessage() override = default;

        void operator()() override
        {
            std::ofstream stream{ this->path, std::fstream::out | std::fstream::binary };
            stream.write(reinterpret_cast<char const *>(msg.data()), msg.size() * sizeof(T));
        }
    };

    template<typename T>
    class MessageHandler
    {
    private:
        std::unique_ptr<Message<T>> msg;

    public:
        MessageHandler(std::unique_ptr<Message<T>> &&msg) : msg{ std::move(msg) } {}
        MessageHandler(MessageHandler const&) = delete;
        MessageHandler(MessageHandler&&) = default;
        MessageHandler& operator=(MessageHandler const&) = delete;
        MessageHandler& operator=(MessageHandler&&) = default;
        ~MessageHandler() = default;

        void operator()()
        {
            (*msg)();
        }
    };

    template<typename T>
    class ThreadStream
    {
    private:
        static ThreadPool<MessageHandler<T>> pool;
    public:
        ThreadStream() { if (!pool.IsRunning()) { pool.Start(); } };
        ThreadStream(ThreadStream const&) = delete;
        ThreadStream(ThreadStream&&) = delete;
        ThreadStream& operator=(ThreadStream const&) = delete;
        ThreadStream& operator=(ThreadStream&&) = delete;
        ~ThreadStream() = default;

        void Write(std::filesystem::path &&path, std::vector<T> &&data)
        {
            std::unique_ptr<Message<T>> msg{ std::make_unique<WriteMessage<T>>(std::move(data), std::move(path)) };
            MessageHandler<T> out{ std::move(msg) };
            pool.Append(std::move(out));
        }
    };

    template<typename T>
    inline ThreadPool<MessageHandler<T>> ThreadStream<T>::pool{ 1 };
}

#endif