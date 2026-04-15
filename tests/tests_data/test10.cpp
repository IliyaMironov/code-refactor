class Logger {
public:
    virtual void log() {}
    ~Logger() {}
};

class FileLogger : public Logger {};

class Handler {
public:
    virtual void handle() {}
    ~Handler() {}
};

class EventHandler : public Handler {};

class Standalone {
public:
    ~Standalone() {}
};
