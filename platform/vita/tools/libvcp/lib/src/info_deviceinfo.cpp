#include "info_deviceinfo.hpp"
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
    struct deviceInfo_struct {
        uint32_t unk0; // min. coredump version
        uint32_t unk4; // zeroes

        uint32_t audio_exclusive_owner_pid; // 10
        uint32_t audio_main_out_owner_pid; // 11
        uint32_t audio_bgm_out_owner_pid; // 12
        uint32_t audio_voice_out_owner_pid; // 13
        uint32_t audio_in_owner_pid; // 14

        uint32_t display_back_port_owner_pid; // 15
        uint32_t input_device_owner_pid; // 16
        uint32_t bluetooth_controller_owner_appid; // 17
        uint32_t shell_pid; // 18
        uint32_t active_app_appid; // 19

        uint32_t display_owner_pid0; // 3A
        uint32_t display_owner_pid1; // 3B
        uint32_t system_chat_owner_appid; // 3C
        uint32_t back_render_port_owner_appid; // 3D
        uint32_t render_pids_count;
    //    array of render pids // 1A-29
    //    int32_t touch_pids_count;
    //    array of touch pids //  // 2A - 39
    //    int32_t zeroes[30]; // padding?
    };
*/

    deviceInfo::deviceInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf, size);
        in.seekg(8, std::ios_base::beg);

        read_var(_audio_owner_pids.exclusive);

        read_var(_audio_owner_pids.main_out);
        read_var(_audio_owner_pids.bgm_out);
        read_var(_audio_owner_pids.voice_out);
        read_var(_audio_owner_pids.in);

        read_var(_display_back_port_owner_pid);
        read_var(_input_device_owner_pid);
        read_var(_bluetooth_controller_owner_appid);
        read_var(_shell_pid);
        read_var(_active_app_appid);
        read_var(_display_owner_pid[0]);
        read_var(_display_owner_pid[1]);
        read_var(_system_chat_owner_appid);
        read_var(_back_render_port_owner_appid);

        uint32_t render_cnt;
        read_var(render_cnt);

        for (uint32_t i = 0; i < render_cnt; ++i)
        {
            uint32_t pid;
            read_var(pid);
            _render_pids.push_back(pid);
        }

        uint32_t touch_cnt;
        read_var(touch_cnt);

        for (uint32_t i = 0; i < touch_cnt; ++i)
        {
            uint32_t pid;
            read_var(pid);
            _touch_pids.push_back(pid);
        }
    }

    ::vita::coredump::audioOwnerPids& deviceInfo::audioOwnerPids()
    {
        return _audio_owner_pids;
    }

    std::array<uint32_t,2>& deviceInfo::displayOwnerPid()
    {
        return _display_owner_pid;
    }

    uint32_t deviceInfo::displayBackPortOwnerPid()
    {
        return _display_back_port_owner_pid ;
    }

    uint32_t deviceInfo::inputDeviceOwnerPid()
    {
        return _input_device_owner_pid;
    }

    uint32_t deviceInfo::shellPid()
    {
        return _shell_pid;
    }

    uint32_t deviceInfo::activeAppAppid()
    {
        return _active_app_appid;
    }

    uint32_t deviceInfo::systemChatOwnerAppid()
    {
        return _system_chat_owner_appid;
    }

    uint32_t deviceInfo::backRenderPortOwnerAppid()
    {
        return _back_render_port_owner_appid;
    }

    uint32_t deviceInfo::bluetoothControllerOwnerAppid()
    {
        return _bluetooth_controller_owner_appid;
    }

    std::vector<uint32_t>& deviceInfo::renderPids()
    {
        return _render_pids;
    }

    std::vector<uint32_t>& deviceInfo::touchPids()
    {
        return _touch_pids;
    }

}; // coredump
}; // vita

