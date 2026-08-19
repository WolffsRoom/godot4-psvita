#ifndef MEMSTREAM_H
#define MEMSTREAM_H

#include <streambuf>
#include <iosfwd>
#include <ios>
#include <istream>
#include <ostream>

template <typename ch, typename tr> 
class baseMemstreamBuf: public std::basic_streambuf<ch, tr>
{
    protected:
        typedef std::basic_streambuf<ch, tr> Base;
        typedef std::basic_ios<ch, tr> IOS;
        typedef ch char_type;
        typedef tr char_traits;
        typedef typename Base::int_type int_type;
        typedef typename Base::pos_type pos_type;
        typedef typename Base::off_type off_type;

    public:
        baseMemstreamBuf(char_type* pBuffer, std::streamsize bufferSize) : _pBuffer(pBuffer), _bufferSize(bufferSize)
        {
            this->setg(_pBuffer, _pBuffer, _pBuffer + _bufferSize);
            this->setp(_pBuffer, _pBuffer + _bufferSize);
        }

        ~baseMemstreamBuf()
        {
        }

        virtual int_type overflow(int_type /*c*/)
        {
            return char_traits::eof();
        }

        virtual int_type underflow()
        {
            return char_traits::eof();
        }

        virtual pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
        {
            const pos_type fail = off_type(-1);
            off_type newoff = off_type(-1);

            if ((which & std::ios_base::in) != 0)
            {
                if (this->gptr() == 0)
                    return fail;

                if (way == std::ios_base::beg)
                {
                    newoff = 0;
                }
                else if (way == std::ios_base::cur)
                {
                    // cur is not valid if both in and out are specified (Condition 3)
                    if ((which & std::ios_base::out) != 0)
                        return fail;
                    newoff = this->gptr() - this->eback();
                }
                else if (way == std::ios_base::end)
                {
                    newoff = this->egptr() - this->eback();
                }

                if ((newoff + off) < 0 || (this->egptr() - this->eback()) < (newoff + off))
                    return fail;
                this->setg(this->eback(), this->eback() + newoff + off, this->egptr());
            }

            if ((which & std::ios_base::out) != 0)
            {
                if (this->pptr() == 0)
                    return fail;

                if (way == std::ios_base::beg)
                {
                    newoff = 0;
                }
                else if (way == std::ios_base::cur)
                {
                    // cur is not valid if both in and out are specified (Condition 3)
                    if ((which & std::ios_base::in) != 0)
                        return fail;
                    newoff = this->pptr() - this->pbase();
                }
                else if (way == std::ios_base::end)
                {
                    newoff = this->epptr() - this->pbase();
                }

                if (newoff + off < 0 || (this->epptr() - this->pbase()) < newoff + off)
                    return fail;
                this->pbump((int)(newoff + off - (this->pptr() - this->pbase())));
            }

            return newoff;
        }

        virtual int sync()
        {
            return 0;
        }

        std::streamsize charsWritten() const
        {
            return static_cast<std::streamsize>(this->pptr() - this->pbase());
        }

        void reset()
        {
            this->setg(_pBuffer, _pBuffer, _pBuffer + _bufferSize);
            this->setp(_pBuffer, _pBuffer + _bufferSize);
        }

    private:
        char_type*      _pBuffer;
        std::streamsize _bufferSize;

        baseMemstreamBuf();
        baseMemstreamBuf(const baseMemstreamBuf&);
        baseMemstreamBuf& operator = (const baseMemstreamBuf&);
};


typedef baseMemstreamBuf<char, std::char_traits<char>> memstreamBuf;

class memios: public virtual std::ios
{
    public:
        memios(char* pBuffer, std::streamsize bufferSize) : _buf(pBuffer, bufferSize)
        {
            init(&_buf);
        }

        ~memios() {}

        memstreamBuf* rdbuf()
        {
            return &_buf;
        }

    protected:
        memstreamBuf _buf;
};


class memstream: public memios, public std::istream
{
    public:
        memstream(const char* pBuffer, std::streamsize bufferSize) : memios(const_cast<char*>(pBuffer), bufferSize), std::istream(&_buf) { }
        ~memstream() {}
};

// macros
#define read_var(n) in.read((char*)&n, sizeof(n));
#define read_string(n, l) \
    do { \
        n = std::string(""); \
        char buf[l]; \
        char* ptr = (char*)buf; \
        in.read(buf, l); \
        while (*ptr == 0 && ptr < (char*)buf + l) { \
            ptr++; \
        } \
        if (ptr >= (char*)buf + l) break; \
        n = std::string(ptr); \
    } while(0)

#define read_vector(n, l) n.resize(l); \
                          in.read((char*)n.data(), l);

#define read_array(n, l) in.read((char*)n.data(), l);



#endif
