#ifndef SERIALISABLE_HPP_INCLUDED
#define SERIALISABLE_HPP_INCLUDED

#include <nlohmann/json.hpp>
#include <type_traits>
#include <vec/vec.hpp>

struct serialisable
{
    virtual void serialise(nlohmann::json& data, bool encode){}

    static size_t time_ms();

    virtual ~serialisable();
};

inline
size_t get_next_persistent_id()
{
    thread_local static size_t gpid = 0;

    return gpid++;
}

struct owned
{
    size_t pid = get_next_persistent_id();
};

template<int N, typename T>
inline
void do_serialise(nlohmann::json& data, vec<N, T>& in, const std::string& name, bool encode)
{
    if(encode)
    {
        for(int i=0; i < N; i++)
        {
            data[name][i] = in.v[i];
        }
    }
    else
    {
        if(data.count(name) == 0)
        {
            in = vec<N, T>();
        }
        else
        {
            for(int i=0; i < N; i++)
            {
                in.v[i] = data[name][i];
            }
        }
    }
}

template<typename T>
void do_serialise(nlohmann::json& data, T& in, const std::string& name, bool encode)
{
    if constexpr(std::is_base_of_v<serialisable, T>)
    {
        in.serialise(data[name], encode);
    }

    if constexpr(!std::is_base_of_v<serialisable, T>)
    {
        if(encode)
        {
            data[name] = in;
        }
        else
        {
            if(data.find(name) == data.end())
            {
                in = T();
            }
            else
            {
                in = data[name];
            }
        }
    }
}

template<typename T>
void do_serialise(nlohmann::json& data, T*& in, const std::string& name, bool encode)
{
    assert(in);

    do_serialise(data, *in, name, encode);
}

template<typename T>
void do_serialise(nlohmann::json& data, std::vector<T>& in, const std::string& name, bool encode)
{
    if(encode)
    {
        for(int i=0; i < (int)in.size(); i++)
        {
            do_serialise(data[name], in[i], std::to_string(i), encode);
        }
    }
    else
    {
        in = std::vector<T>();

        std::map<int, nlohmann::json> dat;

        for(auto& info : data[name].items())
        {
            dat[std::stoi(info.key())] = info.value();
        }

        for(int i=0; i < (int)dat.size(); i++)
        {
            T next = T();
            do_serialise(data[name], next, std::to_string(i), encode);

            in.push_back(next);
        }
    }
}


#define DO_SERIALISE(x){do_serialise(data, x, std::string(#x), encode);}

struct test_serialisable : serialisable
{
    virtual void serialise(nlohmann::json& data, bool encode) override;

    int test_datamember = 0;
};

template<typename T>
nlohmann::json serialise(T& in)
{
    nlohmann::json ret;

    if constexpr(std::is_base_of_v<serialisable, T>)
    {
        in.serialise(ret, true);
    }

    if constexpr(!std::is_base_of_v<serialisable, T>)
    {
        ret = in;
    }

    return ret;
}

template<typename T>
T deserialise(nlohmann::json& in)
{
    T ret;

    if constexpr(std::is_base_of_v<serialisable, T>)
    {
        ret.serialise(in, false);
    }

    if constexpr(!std::is_base_of_v<serialisable, T>)
    {
        ret = (T)in;
    }

    return ret;
}

template<typename T>
void deserialise(nlohmann::json& in, T& dat)
{
    if constexpr(std::is_base_of_v<serialisable, T>)
    {
        dat.serialise(in, false);
    }

    if constexpr(!std::is_base_of_v<serialisable, T>)
    {
        dat = (T)in;
    }
}

#endif // SERIALISABLE_HPP_INCLUDED
