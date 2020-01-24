#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <optional>
#include <streambuf>

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
            Node<T> *last;
            Node<T> *null{ nullptr };
            do
            {
                last = Last();
            } while (!last->next.compare_exchange_weak(null, item));
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
			Node<T> *node;
			Node<T> *tail;
			do
			{
				node = head.load();
				if (node == nullptr)
					return nullptr;
				tail = node->Tail();
			} while (!head.compare_exchange_strong(node, tail));
            T* val{ node->Value() };
            auto wait{ used.load() };

            while (wait > 0) {
                std::this_thread::yield();
                wait = used.load();
            }

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
		std::condition_variable cnd;
		std::mutex mtx;

        void Process()
        {
            bool check{ run.load() };
            while (check)
            {
                if (commands.Empty())
                {
					std::unique_lock<std::mutex> lck{ mtx };
					cnd.wait(lck);
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
		ThreadPool(std::size_t threads) : threads{ threads }, commands{}, run{ false }, cnd{}, mtx{} {}
        ThreadPool(ThreadPool const&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool const&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;
        ~ThreadPool()
        {			
            Join();
        }

        void Stop()
        {
            run.store(false);
			cnd.notify_all();
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
			cnd.notify_all();
        }

        void Append(T const &fn)
        {
            commands.Append(fn);
			cnd.notify_all();
        }

        void Join() {
            while (!commands.Empty()) {
                std::this_thread::yield();
            }
            Stop();
            for (std::size_t i = 0; i < threads.size(); ++i)
            {
                run.store(false);
                cnd.notify_all();
                if (threads[i].joinable())
                {
                    threads[i].join();
                }
            }
        }
    };

	template<typename T>
	class Future
	{
	private:
		T value;
		std::atomic<bool> shared;
	public:
		template<typename... Args>
		Future(Args... args) : value{ T{std::forward<Args>(args)...} }, shared{ false } {}
		Future(Future const&) = delete;
		Future(Future&&) = default;
		Future& operator=(Future const&) = delete;
		Future& operator=(Future&&) = default;
		virtual ~Future() = default;

		std::optional<T> Get()
		{
			bool expected{ true };
			if (!shared.compare_exchange_strong(expected, false))
				return std::nullopt;
			return std::make_optional<T>(std::move(value));
		}

		bool IsEmpty()
		{
			return !(shared.load());
		}

		void Set(T &&val)
		{
			bool expected{ false };
			if (!shared.compare_exchange_strong(expected, true))
				return;
			value = std::move(val);
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

        std::filesystem::path tempFile(std::filesystem::path const& path) {
            auto out = path;
            out.replace_filename(path.filename().replace_extension(".tmp"));
            return out;
        }        
    public:
        WriteMessage(std::vector<T> &&message, std::filesystem::path &&path) : Message<T>{ std::move(path) }, msg{ std::move(message) } {}
        WriteMessage(WriteMessage const&) = delete;
        WriteMessage(WriteMessage&&) = default;
        WriteMessage& operator=(WriteMessage const&) = delete;
        WriteMessage& operator=(WriteMessage&&) = default;
        ~WriteMessage() override = default;

        void operator()() override
        {
            auto tmp = tempFile(this->path);
            std::ofstream stream{ tmp, std::fstream::out | std::fstream::binary };
            stream.write(reinterpret_cast<char const *>(msg.data()), msg.size() * sizeof(T));
			stream.close();
            std::filesystem::rename(tmp, this->path);
        }
    };

	template<typename T>
	class ReadMessage : public Message<T>
	{
	private:
		Future<std::vector<T>>& msg;
	public:
		ReadMessage(Future<std::vector<T>> &message, std::filesystem::path &&path) : Message<T>{ std::move(path) }, msg{ message } {}
		ReadMessage(ReadMessage const&) = delete;
		ReadMessage(ReadMessage&&) = default;
		ReadMessage& operator=(ReadMessage const&) = delete;
		ReadMessage& operator=(ReadMessage&&) = default;
		~ReadMessage() override = default;

		void operator()() override
		{
			std::ifstream stream{ this->path, std::fstream::in | std::fstream::binary };
			std::vector<T> out{};
			std::istreambuf_iterator<char> begin{ stream };
			std::istreambuf_iterator<char> end{};
			while(begin != end)
			{
				std::array<char, sizeof(T)> item{};
				for (std::size_t i = 0; i < sizeof(T); ++i)
				{
					item[i] = *begin;
					++begin;
				}
				T* ptr{ reinterpret_cast<T*>(item.data()) };
				out.push_back(*ptr);
			}
			stream.close();
			msg.Set(std::move(out));
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

		void Read(std::filesystem::path &&path, Future<std::vector<T>> &data)
		{
			std::unique_ptr<Message<T>> msg{ std::make_unique<ReadMessage<T>>(data, std::move(path)) };
			MessageHandler<T> out{ std::move(msg) };
			pool.Append(std::move(out));
		}
    };

    template<typename T>
    inline ThreadPool<MessageHandler<T>> ThreadStream<T>::pool{ 1 };
}

#endif