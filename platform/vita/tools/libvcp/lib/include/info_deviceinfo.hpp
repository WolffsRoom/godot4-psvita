#ifndef VITA_COREDUMP_DEVICEINFO_H
#define VITA_COREDUMP_DEVICEINFO_H

#include <cstdint>
#include <vector>
#include <array>

namespace vita
{
namespace coredump
{

typedef struct
{
    uint32_t exclusive;
    uint32_t main_out;
    uint32_t bgm_out;
    uint32_t voice_out;
    uint32_t in;
} audioOwnerPids;

class deviceInfo {
    public:
        deviceInfo(void* buf, uint32_t size);

        ::vita::coredump::audioOwnerPids& audioOwnerPids();
        std::array<uint32_t,2>& displayOwnerPid();
        uint32_t displayBackPortOwnerPid();
        uint32_t inputDeviceOwnerPid();
        uint32_t shellPid();

        uint32_t activeAppAppid();
        uint32_t systemChatOwnerAppid();
        uint32_t backRenderPortOwnerAppid();
        uint32_t bluetoothControllerOwnerAppid();

        std::vector<uint32_t>& renderPids();
        std::vector<uint32_t>& touchPids();


    private:
        ::vita::coredump::audioOwnerPids _audio_owner_pids;

        uint32_t _display_back_port_owner_pid;
        uint32_t _input_device_owner_pid;
        uint32_t _bluetooth_controller_owner_appid;
        uint32_t _shell_pid;
        uint32_t _active_app_appid;

        std::array<uint32_t,2> _display_owner_pid;
        uint32_t _system_chat_owner_appid;
        uint32_t _back_render_port_owner_appid;
        std::vector<uint32_t> _render_pids;
        std::vector<uint32_t> _touch_pids;
}; // deviceInfo

}; // coredump
}; // vita

#endif