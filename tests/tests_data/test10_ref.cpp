class Logger {
public:
    virtual void log() {}
    virtual ~Logger() {}
};

class FileLogger : public Logger {};

class Handler {
public:
    virtual void handle() {}
    virtual ~Handler() {}
};

class EventHandler : public Handler {};

class Standalone {
public:
    ~Standalone() {}
};
