#ifndef ADDRESS_HPP
#define ADDRESS_HPP

class Address {
    public:
        Address();

        Address( unsigned char a, 
                unsigned char b, 
                unsigned char c, 
                unsigned char d, 
                unsigned short port);

        Address(unsigned int address, unsigned short port);

        unsigned int getAddress() const;

        unsigned short getPort() const;

    private:
        unsigned int address;
        unsigned short port;
        unsigned int createAddress(
                unsigned char a, 
                unsigned char b, 
                unsigned char c, 
                unsigned char d);
};

#endif
