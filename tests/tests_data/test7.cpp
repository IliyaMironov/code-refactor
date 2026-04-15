class Printable {
public:
    virtual void print() {}
    ~Printable() {}
};

class Serializable {
public:
    virtual void serialize() {}
    ~Serializable() {}
};

class Document : public Printable, public Serializable {
public:
    void print() {}
    void serialize() {}
};
