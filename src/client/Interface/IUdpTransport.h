class IUdpTransport
{
public:
    virtual ~IUdpTransport() = default;

    virtual void send(const asio::ip::udp::endpoint&, std::span<const uint8_t>) = 0;
};
