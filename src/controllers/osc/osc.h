#ifndef OSC_h
#define OSC_h


// IWYU pragma: no_include <bits/stdint-intn.h>
// IWYU pragma: no_include <bits/stdint-uintn.h>
#include <cstdint> // IWYU pragma: keep

#include <algorithm>    // for reverse, find, transform
#include <chrono>       // for seconds
#include <functional>   // for function
#include <iostream>     // for size_t, operator<<, endl, basic_ostream, cerr
#include <optional>
#include <string>       // for string, allocator, operator==, basic_string
#include <thread>       // for thread
#include <type_traits>  // for endian, endian::little, endian::native, remov...
#include <utility>      // for move, pair
#include <variant>      // for variant
#include <vector>       // for vector, vector<>::iterator

#include <QHostAddress>
#include <QSysInfo>
#include <QUdpSocket>

class OSC final : public QObject {
  Q_OBJECT
  public:
    struct Message {
      public:
        enum class Type {
          i, f, s, b
        };
        static_assert(sizeof(float) == 4, "Float is not 32 bits");
        using Argument = std::variant<int32_t, float, std::string, std::vector<char>>;
        std::string addressPattern;
        std::vector<Type> types;
        std::vector<Argument> arguments;
        Message(std::string addressPattern, std::vector<Type> types, std::vector<Argument> arguments)
          : addressPattern(std::move(addressPattern)), types(std::move(types)), arguments(std::move(arguments)) {}
      private:
        template <std::size_t n, typename It>
        static auto arrayFromIt(It it) {
            auto res = std::array<std::remove_cv_t<std::remove_reference_t<decltype(*it)>>, n>{};
            std::copy(it, it + n, res.begin());
            return res;
        }

        template <std::size_t n, typename It>
        static auto getArrayAndAdvanceIt(It& it) {
            return arrayFromIt<n>(std::exchange(it, it + n));
        }

        template <typename T, std::size_t n>
        static auto arrayToVect(std::array<T, n> xs) -> std::vector<T> {
            return {xs.begin(), xs.end()};
        }

        template <typename T> static constexpr Type forType() = delete;

        template <typename T>
        static std::array<char, sizeof(T)> makeOSCnum(T v) {
            char* p = (char*)&v;
            auto res = arrayFromIt<sizeof(T)>(p);
            if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::LittleEndian) {
                std::reverse(res.begin(), res.end());
            }
            return res;
        }

        template <typename T>
        static T unmakeOSCnum(std::array<char, sizeof(T)> v) {
            if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::LittleEndian) {
              std::reverse(v.begin(), v.end());
            }
            return *(T*)v.begin();
        }

        template <typename T, typename It>
        static T unmakeOSCnum(It& it) {
            return unmakeOSCnum<T>(getArrayAndAdvanceIt<sizeof(T)>(it));
        }

        template <typename T>
        static std::vector<char> makeOSCstring(T str) {
            std::size_t pad = 4 - (str.size() % 4);
            std::vector<char> v(str.begin(), str.end());
            for(std::size_t i = 0; i < pad; i++) {
                v.push_back('\0');
            }
            return v;
        }

        static std::vector<char> makeTypeTagString(std::vector<Type> types);
        static std::vector<char> makeArgument(Argument arg);
        static std::vector<char> makeArguments(std::vector<Argument> arg);
      public:
        explicit Message(std::string addressPattern) : addressPattern(std::move(std::move(addressPattern))) {}
        template <typename T, typename... Args>
          Message(std::string addressPattern, T arg, Args... args) : Message(std::move(addressPattern), args...) {
            types.insert(types.begin(), forType<T>());
            arguments.insert(arguments.begin(), arg);
          }
      private:
        template <typename It>
          static std::optional<Message> parse(It begin, It end) {
            auto addressPatternEnd = std::find(begin, end, '\0');
            std::string addressPattern(begin, addressPatternEnd);
            std::size_t addressPatternPad = 4 - ((addressPatternEnd - begin) % 4);
            auto typeTagStringBegin = addressPatternEnd + addressPatternPad;
            if (*typeTagStringBegin != ',') {
              std::cerr << "Missing Type Tag String" << std::endl;
              return std::nullopt;
            }
            auto typeTagStringEnd = std::find(typeTagStringBegin, end, '\0');
            std::vector<Type> types(typeTagStringEnd - (typeTagStringBegin + 1));

            struct invalidTypeTag {};
            try {
              std::transform(typeTagStringBegin + 1, typeTagStringEnd, types.begin(), [](char t){ switch (t) {
                case 'i': return Type::i;
                case 'f': return Type::f;
                case 's': return Type::s;
                case 'b': return Type::b;
                default: throw invalidTypeTag{};
              }});
            } catch (invalidTypeTag const & x) {
              return std::nullopt;
            }
            std::size_t typeTagStringPad = 4 - ((typeTagStringEnd - typeTagStringBegin) % 4);
            char* argumentsIt = &*(typeTagStringEnd + typeTagStringPad);
            std::vector<Argument> arguments;
            for (Type t : types) {
              switch (t) {
                case Type::i:
                  arguments.emplace_back(unmakeOSCnum<int32_t>(argumentsIt));
                  break;
                case Type::f:
                  arguments.emplace_back(unmakeOSCnum<float>(argumentsIt));
                  break;
                case Type::s:
                  {
                    std::string str(argumentsIt);
                    arguments.emplace_back(str);
                    std::size_t n = str.size();
                    std::size_t pad = 4 - (n % 4);
                    argumentsIt += n + pad;
                  }
                  break;
                case Type::b:
                  {
                    int32_t n = *reinterpret_cast<int32_t*>(argumentsIt);
                    argumentsIt += sizeof(int32_t);
                    std::vector<char> v(argumentsIt, argumentsIt + n);
                    arguments.emplace_back(v);
                    std::size_t pad = 4 - (n % 4);
                    argumentsIt += n + pad;
                  }
                  break;
              }
            }
            return Message(std::move(addressPattern), std::move(types), std::move(arguments));
          }

      public:
        template <typename It>
          static std::vector<Message> getMessages(It begin, It end) {
            if (std::string(begin, begin + 8) == "#bundle") {
              auto it = begin + 16;
              std::vector<OSC::Message> messages;
              while (it < end) {
                auto size = unmakeOSCnum<uint32_t>(it);
                auto newMessages = getMessages(it, it + size);
                messages.insert(messages.end(), newMessages.begin(), newMessages.end());
                it += size;
              }
              return messages;
            }
            if (auto m = Message::parse(begin, end)) {
              return std::vector({*m});
            }
            return std::vector<Message>();
          }

        static auto getMessages(std::vector<char> packet) -> std::vector<Message>;
        auto toPacket() const -> std::vector<char>;
        auto toFloat() const -> std::optional<float>;
        auto debugPayload() const -> std::string;
    };
  private:
    const unsigned short sendPort;
    const QHostAddress address;
    QUdpSocket socket = QUdpSocket{};

    void recvHandler();

    std::vector<std::thread> periodicSends;

  signals:
    void receive(Message const &);

  public:
    OSC(const std::string& ip, unsigned short sendPort, unsigned short recvPort);
    void send(Message const & message);
};

template <> constexpr OSC::Message::Type OSC::Message::forType<int>              () { return Type::i; }
template <> constexpr OSC::Message::Type OSC::Message::forType<float>            () { return Type::f; }
template <> constexpr OSC::Message::Type OSC::Message::forType<std::string>      () { return Type::s; }
template <> constexpr OSC::Message::Type OSC::Message::forType<std::vector<char>>() { return Type::b; }

#endif

