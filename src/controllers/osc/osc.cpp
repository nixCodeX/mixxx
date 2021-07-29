// IWYU pragma: no_include <asio/buffer.hpp>
// IWYU pragma: no_include <asio/ip/address_v4.hpp>
// IWYU pragma: no_include <asio/ip/impl/address.ipp>

#include "osc.h"
#include "moc_osc.cpp"
#include <chrono>        // for duration, steady_clock, operator+, seconds
#include <iterator>      // for back_insert_iterator, back_inserter
#include <new>           // for operator new
#include <numeric>       // for accumulate
#include <ratio>         // for ratio
#include <system_error>  // for error_code
#include <thread>        // for thread, sleep_until

#include <QNetworkDatagram>

template <typename Ret, typename Var, typename Fs, std::size_t I>
auto visit_helper(Var var, Fs fs) -> Ret {
    return std::get<I>(fs)(*std::get_if<I>(&var));
}

template <typename Ret, typename Var, typename Fs, std::size_t ...Is>
auto make_visit_helper_array
    ( std::index_sequence<Is...>
    ) -> std::array<Ret(*)(Var, Fs), std::tuple_size_v<Fs>> {
    return {&visit_helper<Ret, Var, Fs, Is>...};
}

template <typename Ret, typename ...Ts, typename ...Fs>
auto my_visit(std::variant<Ts...> var, Fs ...fs) {
    static_assert
        ( sizeof...(Ts) == sizeof...(Fs)
        , "There must be the same number of visiting functions as cases in the variant"
        );
    static_assert
        ( (std::is_convertible_v<std::invoke_result_t<Fs, Ts>, Ret> && ...)
        , "Function return values must all be convertible to Ret"
        );

    auto gs = make_visit_helper_array
        <Ret, std::variant<Ts...>, std::tuple<Fs...>>
        (std::index_sequence_for<Fs...>{});

    return gs[var.index()](std::move(var), std::tuple{fs...});
}

std::vector<char> OSC::Message::makeTypeTagString(std::vector<Type> types) {
    std::string str = ",";
    for (Type t : types) {
        switch (t) {
            case Type::i: str += "i"; break;
            case Type::f: str += "f"; break;
            case Type::s: str += "s"; break;
            case Type::b: str += "b"; break;
        }
    }
    return makeOSCstring(str);
}

template <typename It>
std::vector<char> concatIt(It data) {
    std::size_t n = std::accumulate(data.begin(), data.end(), 0, [](std::size_t a, std::vector<char> b){ return a + b.size(); });
    std::vector<char> v(n);
    auto it = v.begin();
    for (std::vector<char> d : data) {
        std::copy(d.begin(), d.end(), it);
        it += d.size();
    }
    return v;
}

template <typename It, typename... Vs>
void copyDatas(It it) { (void)it; }
template <typename It, typename V, typename... Vs>
void copyDatas(It it, V v, Vs... vs) {
    std::copy(v.begin(), v.end(), it);
    copyDatas(it + v.size(), vs...);
}

template <typename... Vs>
std::vector<char> concat(Vs... vs) {
    std::size_t n = (vs.size() + ...);
    std::vector<char> v(n);
    copyDatas(v.begin(), vs...);
    return v;
}

std::vector<char> OSC::Message::makeArgument(Argument arg) {
    return my_visit<std::vector<char>>
        ( std::move(arg)
        , [](int32_t x) { return arrayToVect(makeOSCnum(x)); }
        , [](float x) { return arrayToVect(makeOSCnum(x)); }
        , &makeOSCstring<std::string>
        , [](std::vector<char> v) -> std::vector<char> {
            return makeOSCstring(concat(makeOSCnum<int32_t>(v.size()), v));
          }
        );
}

std::vector<char> OSC::Message::makeArguments(std::vector<Argument> arg) {
    std::vector<std::vector<char>> data;
    std::transform(arg.begin(), arg.end(), std::back_inserter(data), makeArgument);
    return concatIt(data);
}

auto OSC::Message::toPacket() const -> std::vector<char> {
    return concat(
        makeOSCstring(addressPattern),
        makeTypeTagString(types),
        makeArguments(arguments)
    );
}

auto OSC::Message::toFloat() const -> std::optional<float> {
    if (arguments.size() != 1) {
        std::cerr << "Wrong number of arguments in packet" << std::endl;
        return std::nullopt;
    }
    return my_visit<std::optional<float>>
        ( arguments[0]
        , [](int32_t x) -> float { return x; }
        , [](float x) { return x; }
        , [](std::string) { return std::nullopt; }
        , [](std::vector<char>) { return std::nullopt; }
        );
}

auto OSC::Message::debugPayload() const -> std::string {
    auto ret = std::string{};
    for (auto& arg : arguments) {
        ret += my_visit<std::string>
            ( arg
            , [](int32_t x) { return "i: " + std::to_string(x); }
            , [](float x) { return "f: " + std::to_string(x); }
            , [](std::string x) { return "s: \"" + x + '\"'; }
            , [](std::vector<char>) { return "b"; }
            ) + ",";
    }
    return ret;
}

void OSC::recvHandler() {
    while (true) {
        auto datagram = socket.receiveDatagram();
        if (!datagram.isValid()) { return; }

        for (const Message& message : Message::getMessages(datagram.data().begin(), datagram.data().end())) {
            emit receive(message);
        }
    }
}

OSC::OSC(const std::string& ip, unsigned short sendPort, unsigned short recvPort)
    : sendPort{sendPort}
    , address{ip.c_str()}
{
    socket.bind(QHostAddress{"0.0.0.0"}, recvPort);
    connect(&socket, &QUdpSocket::readyRead, this, [=](){ this->recvHandler(); });
}

void OSC::send(Message const & message) {
    auto packet = message.toPacket();
    socket.writeDatagram(packet.data(), packet.size(), address, sendPort);
}

