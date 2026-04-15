class Printable {
public:
    virtual void print() {}
    ~Printable() {}
};

class Serializable {
public:
    virtual void serialize() {}
    virtual ~Serializable() {}
};

class Document : public Printable, public Serializable {
public:
    void print() override {}
    void serialize() override {}
};
