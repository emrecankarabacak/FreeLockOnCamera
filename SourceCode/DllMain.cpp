#pragma once

#include "Custom.h"
#include "Include/CameraFix.cpp"
#include "Include/Ini.h"
#include "Include/ModUtils.h"
#include <sstream>

using namespace Custom;
using namespace mINI;
using namespace ModUtils;

const std::string author = "SchuhBaum";
const std::string version = "0.1.1";

//
// config
//

bool lock_camera = false;
bool is_health_bar_hidden = true;
bool is_lock_on_camera_zoom_enabled = true;
bool is_only_using_camera_yaw = true;
bool is_toggle = true;

float angle_to_camera_score_multiplier = 6000.0F;
float camera_height = 1.45F;
std::string Get_HexString(float f) {
    return Add_Spaces_To_HexString(Swap_HexString_Endian(Convert_Float_To_LowercaseHexString(f)));
}

std::string target_switching_mode = "modded_switch";

//
//
//

void Log_Parameters() {
    Log("angle_to_camera_score_multiplier");
    Log("  float: ", angle_to_camera_score_multiplier, "  hex: ", Get_HexString(angle_to_camera_score_multiplier));
    Log("camera_height");
    Log("  float: ", camera_height, "  hex: ", Get_HexString(camera_height));
    
    Log("lock_camera: ", lock_camera ? "true" : "false");
    Log("is_only_using_camera_yaw: ", is_only_using_camera_yaw ? "true" : "false");
    Log("is_health_bar_hidden: ", is_health_bar_hidden ? "true" : "false");
    Log("is_lock_on_camera_zoom_enabled: ", is_lock_on_camera_zoom_enabled ? "true" : "false");
    Log("is_toggle: ", is_toggle ? "true" : "false");

    Log("target_switching_mode: ", target_switching_mode);
    Log_Separator();
    Log_Separator();
}
    
void ReadAndLog_Config() {
    Log("ReadAndLog_Config");
    Log_Separator();
	INIFile config(GetModFolderPath() + "\\config.ini");
	INIStructure ini;

    try {
        if (!config.read(ini)) {
            Log("The config file was not found. Create a new one.");
            ini["FreeLockOnCamera"]["angle_to_camera_score_multiplier"] = std::to_string(static_cast<int>(angle_to_camera_score_multiplier));
            ini["FreeLockOnCamera"]["camera_height"] = std::to_string(camera_height);
            
            ini["FreeLockOnCamera"]["lock_camera"] = std::to_string(is_health_bar_hidden);
            ini["FreeLockOnCamera"]["is_health_bar_hidden"] = std::to_string(is_health_bar_hidden);
            ini["FreeLockOnCamera"]["is_lock_on_camera_zoom_enabled"] = std::to_string(is_lock_on_camera_zoom_enabled);
            ini["FreeLockOnCamera"]["is_only_using_camera_yaw"] = std::to_string(is_only_using_camera_yaw);
            ini["FreeLockOnCamera"]["is_toggle"] = std::to_string(is_toggle);
            
            ini["FreeLockOnCamera"]["target_switching_mode"] = target_switching_mode;
            config.write(ini, true);
            Log_Parameters();
            return;
        }
        
        // camera_height == 0 crashes the game;
        angle_to_camera_score_multiplier = stoi(ini["FreeLockOnCamera"]["angle_to_camera_score_multiplier"]);
        camera_height = stof(ini["FreeLockOnCamera"]["camera_height"]);
        if (camera_height > -0.01F && camera_height < 0.01F) camera_height = 0.01F;
        
        std::string str;
        str = ini["FreeLockOnCamera"]["lock_camera"];
        std::istringstream(str) >> std::boolalpha >> lock_camera;
        str = ini["FreeLockOnCamera"]["is_health_bar_hidden"];
        std::istringstream(str) >> std::boolalpha >> is_health_bar_hidden;
        str = ini["FreeLockOnCamera"]["is_lock_on_camera_zoom_enabled"];
        std::istringstream(str) >> std::boolalpha >> is_lock_on_camera_zoom_enabled;
        
        str = ini["FreeLockOnCamera"]["is_only_using_camera_yaw"];
        std::istringstream(str) >> std::boolalpha >> is_only_using_camera_yaw;
        str = ini["FreeLockOnCamera"]["is_toggle"];
        std::istringstream(str) >> std::boolalpha >> is_toggle;
        
        str = ini["FreeLockOnCamera"]["target_switching_mode"];
        if (str == "vanilla_switch" || str == "modded_keep" || str == "modded_switch") {
            target_switching_mode = str;
        }
        
        Log_Parameters();
        return;
    } catch(const std::exception& exception) {
        Log("Could not read or create the config file. Use defaults.");
        Log_Parameters();
        return;
    }
}

void Apply_AngleToCameraMod() {
    Log("Apply_AngleToCameraMod");
    Log_Separator();
    
    std::string vanilla;
    std::string modded;
    uintptr_t assembly_location;
    
    // vanilla:
    // uses the normalized camera rotation to determine cos(angle_to_camera);
    // 0f 28 40 30      --  movaps xmm0,[rax+30] 
    // 0f 28 48 40      --  movaps xmm1,[rax+40]
    // 0f 29 45 d0      --  movaps [rbp-30],xmm0
    //
    // modded:
    // use a normalized variable that ignores the height (y = 0); however this variable 
    // is rotated; (x, z) = (0, 1) means west instead of north; cos(angle_to_camera) = 
    // dot_product => the dot product needs to use (-z, x) instead of (x, z) later;
    // 0f 28 40 10      --  movaps xmm0,[rax+10] 
    // 0f 28 48 40      --  movaps xmm1,[rax+40]
    // 0f 29 45 d0      --  movaps [rbp-30],xmm0
    
    // it can be a bit time consuming to do it like this; it is more general; I think
    // I will wait and see how much patches can mess these up;
    vanilla = "0f 28 ? 30 0f 28 ? 40 0f 29 ? d0";
    assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location + 3, "30", "10");
    
    Log_Separator();

    // vanilla:
    // this is part of the dot product calculation; cos(angle_to_camera) = dot(v_1, v_2) 
    // where v_1 = candidate_position - camera_position and v_2 = camera_rotation; 
    // v_2 is modded above; v_1.y (height difference) is modded below; the dot product
    // is modded here; 
    // 0f 28 f2         --  movaps xmm6,xmm2
    // f3 0f 59 75 d0   --  mulss xmm6,[rbp-30]
    // 0f 28 ca         --  movaps xmm1,xmm2
    // 0f c6 ca 55      --  shufps xmm1,xmm2,55
    // f3 0f 59 4d d4   --  mulss xmm1,[rbp-2C]
    // f3 0f 58 f1      --  addss xmm6,xmm1
    // 0f c6 d2 aa      --  shufps xmm2,xmm2,-56
    // f3 0f 59 55 d8   --  mulss xmm2,[rbp-28]
    // f3 0f 58 f2      --  addss xmm6,xmm2
    //
    // modded:
    // rotate v_2_modded = (x, 0, z) to v_2_modded_rotated = (-z, 0, x); use
    // v_2_modded_rotated for the score;
    // 0f 28 ca         --  movaps xmm1,xmm2
    // f3 0f 59 4d d8   --  mulss xmm1,[rbp-28]      --  x_new = -z since subss later;
    // 0f 28 f2         --  movaps xmm6,xmm2
    // 0f c6 f2 55      --  shufps xmm6,xmm2,55
    // f3 0f 59 75 d4   --  mulss xmm6,[rbp-2c]      --  y_new is zero;
    // f3 0f 5c f1      --  subss xmm6,xmm1
    // 0f c6 d2 aa      --  shufps xmm2,xmm2,-56
    // f3 0f 59 55 d0   --  mulss xmm2,[rbp-30]      --  z_new = x;
    // f3 0f 58 f2      --  addss xmm6,xmm2
    vanilla = "0f 28 f2 f3 0f 59 75 d0 0f 28 ca 0f c6 ca 55 f3 0f 59 4d d4 f3 0f 58 f1 0f c6 d2 aa f3 0f 59 55 d8 f3 0f 58 f2";
    modded = "0f 28 ca f3 0f 59 4d d8 0f 28 f2 0f c6 f2 55 f3 0f 59 75 d4 f3 0f 5c f1 0f c6 d2 aa f3 0f 59 55 d0 f3 0f 58 f2";
    assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    
    Log_Separator();

    // // vanilla:
    // // uses the height difference from the candidate to the camera in angle_to_camera;
    // // setting it to zero causes issues with locking onto targets behind cover;
    // // f3 44 0f 5c 45 54        --  subss xmm8,[rbp+54]
    // // f3 44 0f 11 45 44        --  movss [rbp+44],xmm8
    // //
    // // modded:
    // // use the height difference to the player instead; otherwise the lock-on target 
    // // might change simply by moving the camera up and down;
    // // f3 44 0f 10 45 74        --  movss xmm8,[rbp+74]
    // // f3 44 0f 11 45 44        --  movss [rbp+44],xmm8
    // vanilla = "f3 44 0f 5c 45 54 f3 44 0f 11 45 44";
    // modded = "f3 44 0f 10 45 74 f3 44 0f 11 45 44";
    // assembly_location = AobScan(vanilla);
    // if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    
    // Log_Separator();
    
    // vanilla:
    // uses the height difference between the candidate and the camera;
    // 48 8d 55 40                              --  lea rdx,[rbp+40]
    // 48 8d 4d 80                              --  lea rcx,[rbp-80]
    // e8 e2 38 a8 ff                           --  call NormalizeVector(...)
    // 0f 28 10                                 --  xmm2,[rax]
    //    
    // modded:
    // instead of modding it I can change the value before it is normalized (function
    // call) and restore it afterwards; this way the height difference is completely
    // ignored but only for the score; way less janky;
    // f3 0f 10 75 44                           --  movss xmm6,[rbp+44]
    // f3 0f 11 55 44                           --  movss [rbp+44],xmm2     -- xmm2 is a function parameter as well and set to zero
    // 48 8d 55 40                              --  lea rdx,[rbp+40]
    // 48 8d 4d 80                              --  lea rcx,[rbp-80]
    // ff 15 02000000 eb 08 new_call_address    --  call NormalizeVector(...)
    // f3 0f 11 75 44                           --  movss [rbp+44],xmm6
    // 0f 28 10                                 --  xmm2,[rax]
    vanilla = "48 8d 55 40 48 8d 4d 80 e8 e2 38 a8 ff 0f 28 10";
    assembly_location = AobScan(vanilla);
    
    if (assembly_location != 0) {
        // https://stackoverflow.com/questions/40936534/how-to-alloc-a-executable-memory-buffer
        MODULEINFO module_info;
        GetModuleInformation(GetCurrentProcess(), GetModuleHandleA("eldenring.exe"), &module_info, sizeof(module_info));
        LPVOID eldenring_assembly_base = module_info.lpBaseOfDll;
        
        int memory_block_size_in_bytes = 64;
        SYSTEM_INFO system_info;
        GetSystemInfo(&system_info);
        auto const page_size = system_info.dwPageSize;
    
        // prepare the memory in which the machine code will be put (it's not executable yet):
        auto const buffer = VirtualAlloc(nullptr, page_size, MEM_COMMIT, PAGE_READWRITE);
        auto const new_assembly_location = reinterpret_cast<std::uintptr_t>(buffer);
        
        // removes 14 + 2 bytes from assembly_location => jump-back-address is assembly_location + 16;
        Hook(assembly_location, new_assembly_location, 2); 

        vanilla = "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00";
        std::string new_call_address = Add_Spaces_To_HexString(Swap_HexString_Endian(NumberToHexString((ULONGLONG)eldenring_assembly_base + 0x18bf90)));
        modded = "f3 0f 10 75 44 f3 0f 11 55 44 48 8d 55 40 48 8d 4d 80 ff 15 02 00 00 00 eb 08 " + new_call_address + " f3 0f 11 75 44 0f 28 10";
        ReplaceExpectedBytesAtAddress((uintptr_t)buffer, vanilla, modded);
        Hook(new_assembly_location + (modded.size() + 1)/3, assembly_location + 16);

        // mark the memory as executable:
        DWORD dummy;
        VirtualProtect(buffer, memory_block_size_in_bytes, PAGE_EXECUTE_READ, &dummy);
    }
    
    Log_Separator();
    Log_Separator();
}

void Apply_CameraHeightMod() {
    Log("Apply_CameraHeightMod");
    Log_Separator();
    
    // vanilla:
    // the height of the camera aims at the center of the player; for aiming it makes 
    // more sense that the camera aims at the head of the character; the offset is
    // stored in rax+0c and is equal to 1.45f;
    // 48 8b 01         --  mov rax,[rcx]
    // 48 85 c0         --  test rax,rax
    // 74 06            --  je <+06>
    // f3 0f10 40 0c    --  movss xmm0,[rax+0C]
    //
    // modded:
    // set it to camera_height; it is not ideal to do it here since it is read here
    // but not set; I have not found the assembly location where it is set;
    // b8 xx xx xx xx   --  mov eax,camera_height
    // 66 0f6e c0       --  movd xmm0,eax
    // 90 90 90 90      --  4x nop
    std::string vanilla = "48 8b 01 48 85 c0 74 06 f3 0f 10 40 0c";
    std::string modded = "b8 " + Get_HexString(camera_height) + " 66 0f 6e c0 90 90 90 90";
    uintptr_t assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    
    Log_Separator();
    Log_Separator();
}

void Apply_FreeLockOnCameraMod() {
    Log("Apply_FreeLockOnCameraMod");
    
    if (!lock_camera) {
        Log_Separator();
    
        // vanilla:
        // sets the variable that disables the free camera during lock-on to one;
        // c6 81 10030000 01    --  mov byte ptr [rcx+00000310],01
        //
        // modded:
        // sets the same variable to zero instead;
        std::string vanilla = "c6 81 10 03 00 00 01";
        std::string modded = "c6 81 10 03 00 00 00";
        uintptr_t assembly_location = AobScan(vanilla);
        if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    }
    
    if (is_lock_on_camera_zoom_enabled) {
        Log_Separator();
        
        // vanilla:
        // checks if the free lock-on camera is disabled; if yes then some camera paramters
        // get changed;
        // 38 93 10030000       --  cmp [rbx+00000310],dl   <-- dl is always zero(?)
        // 74 26                --  je <+26>                <-- nop this
        //
        // modded:
        // changing the lock-on variable at the beginning has some side effects; the camera
        // zooms out a bit when locking on certain large enemies; this change here tries to
        // enable it again;
        std::string vanilla = "38 93 10 03 00 00 74 26";
        std::string modded = "38 93 10 03 00 00 90 90";
        uintptr_t assembly_location = AobScan(vanilla);
        if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    }
    
    Log_Separator();
    Log_Separator();
}

void Apply_KeepLockOnMod() {
    Log("Apply_KeepLockOnMod");
    Log_Separator();
    
    // vanilla:
    // removes the lock-on when you don't look at the target with the camera;
    // 40 0fb6 ff           --  movzx edi,dil       --  dil holds the value zero
    // 41 0f2f c3           --  comiss xmm0,xmm11
    //    
    // modded:
    // override the variable that tracks if the lock-on should be removed; there are 
    // three locations where this needs to be done;
    // 41 8b ff             --  mov edi,r15d        --  r15d holds the value one
    // 90                   --  nop
    // 41 0f2f c3           --  comiss xmm0,xmm11
    std::string vanilla = "40 0f b6 ff 41 0f 2f c3";
    std::string modded = "41 8b ff 90 41 0f 2f c3";
    uintptr_t assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    
    Log_Separator();

    vanilla = "40 0f b6 ff 0f 2f c8 41 0f 43 ff";
    modded = "41 8b ff 90 0f 2f c8 41 0f 43 ff";
    assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    
    Log_Separator();

    vanilla = "40 0f b6 ff 0f 2f c8 41 0f 47 ff";
    modded = "41 8b ff 90 0f 2f c8 41 0f 47 ff";
    assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);

    Log_Separator();
    Log_Separator();
}

void Apply_LockOnCloseRangeMod() {
    Log("Apply_LockOnCloseRangeMod");
    Log_Separator();
    
    // vanilla:
    // this variable has to do with setting a range value; when you are in close range
    // then the lock-on relies less on the camera direction;
    // f3 0f 58 ca                  --  xmm1,XMM2       
    // f3 0f 11 8e 30 29 00 00      --  dword ptr [RSI + 0x2930],xmm1
    //
    // modded:
    // set this range value to zero;
    // c7 86 30290000 00000000      --  mov [RSI + 0x2930],0
    // 90 90                        --  2x nop
    std::string vanilla = "f3 0f 58 ca f3 0f 11 8e 30 29 00 00";
    std::string modded = "c7 86 30 29 00 00 00 00 00 00 90 90";
    uintptr_t assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
   
    Log_Separator();
    Log_Separator();
}

void Apply_LockOnHealthBarMod() {
    Log("Apply_LockOnHealthBarMod");
    Log_Separator();
    
    // vanilla:
    // shows health bars over the currently locked-on target;
    // 75 18            --  jne <+18>
    // 49 8b 5e 08      --  mov rbx,[r14+08]
    //
    // modded:
    // don't show it by skipping the if-block;
    // eb 18            --  jmp <+18>
    // 49 8b 5e 08      --  mov rbx,[r14+08]
    std::string vanilla = "75 18 49 8b 5e 08";
    std::string modded = "eb 18 49 8b 5e 08";
    uintptr_t assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);

    Log_Separator();
    Log_Separator();
}

void Apply_LockOnScoreMod() {
    Log("Apply_LockOnScoreMod");
    Log_Separator();
    
    // vanilla:
    // uses angle_to_player for the score; this variable is saved at [rbx+64];
    // f3 0f 10 43 64       --  movss xmm0,[rbx+64]
    // f3 0f 10 4b 60       --  movss xmm1,[rbx+60]
    //
    // modded:
    // use angle_to_camera instead of angle_to_player; this means that you lock onto 
    // and switch to candidates that you look at with the camera;
    // f3 0f 10 43 6c       --  movss xmm0,[rbx+6c]
    // f3 0f 10 4b 60       --  movss xmm1,[rbx+60]
    std::string vanilla = "f3 0f 10 43 64 f3 0f 10 4b 60";
    std::string modded = "f3 0f 10 43 6c f3 0f 10 4b 60";
    uintptr_t assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    
    if (angle_to_camera_score_multiplier != 30.0F) {
        Log_Separator();

        // vanilla:
        // the score applies a multiplier on how well the player is facing the candidate;
        // 48 c7 83 4c 29 00 00 00 00 f0 41     --  mov [rbx+294c],(float)30
        //
        // increase the score multiplier for angle_to_player; this means that the range 
        // has less effect and the angle has more on the final score;
        // 48 c7 83 4c 29 00 00 xx xx xx xx     --  mov [rbx+294c],angle_to_camera_score_multiplier
        vanilla = "48 c7 83 4c 29 00 00 00 00 f0 41";
        modded = "48 c7 83 4c 29 00 00 " + Get_HexString(angle_to_camera_score_multiplier);
        assembly_location = AobScan(vanilla);
        if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    }

    Log_Separator();
    Log_Separator();
}

void Apply_LockOnSensitivityMod() {
    Log("Apply_LockOnSensitivityMod");
    Log_Separator();
    
    // vanilla:
    // switching locked-on targets requires the mouse to be moved faster than a threshold speed;
    // 72 3a                --  jb <current_address+3a>
    // 0f2f 15 9abe2c02     --  comiss xmm2,<current_address+022cb39a>
    // 76 31                --  jna <current_address+31>                <-- nop this
    //
    // modded:
    // remove the jump when the threshold is not met; this is still bad since it 
    // reacts to moving the mouse rather than the exact camera position; too janky
    // for my taste;
    std::string vanilla = "72 3a 0f 2f 15 9a be 2c 02 76 31";
    std::string modded = "72 3a 0f 2f 15 9a be 2c 02 90 90";
    uintptr_t assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    
    Log_Separator();
    Log_Separator();
}

void Apply_LockOnToggleMod() {
    Log("Apply_LockOnToggleMod");
    Log_Separator();
    
    // vanilla:
    // you have to press the lock-on key every time you lose it; this is not great when
    // you can move the camera freely;
    // 88 86 31 28 00 00        --  mov [rsi+00002831],al
    // 8b 0d 7a 52 ea 03        --  mov ecx,<address_offset>
    //
    // modded:
    // make it a toggle instead => prevent it from getting overriden; still not perfect
    // since you have to remember if it is toggled on or off;
    // 90 90 90 90 90 90        --  6x nop
    // 8b 0d 7a 52 ea 03        --  mov ecx,<address_offset>
    std::string vanilla = "88 86 31 28 00 00 8b 0d 7a 52 ea 03";
    std::string modded = "90 90 90 90 90 90 8b 0d 7a 52 ea 03";
    uintptr_t assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    
    Log_Separator();

    // vanilla:
    // you can only toggle the lock-on off when you currently are locked-on;
    // 80 b9 30 28 00 00 00     --  cmp byte ptr [rcx+00002830],00
    // 0f 94 c0                 --  sete al
    //
    // modded:
    // ignore the variable that checks if you are currently locked-on and
    // instead check the toggle variable;
    // 80 b9 31 28 00 00 00     --  cmp byte ptr [rcx+00002831],00
    // 0f 94 c0                 --  sete al
    vanilla = "80 b9 30 28 00 00 00 0f 94 c0";
    modded = "80 b9 31 28 00 00 00 0f 94 c0";
    assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    
    Log_Separator();

    // vanilla:
    // you lose your toggle after performing a critical hit;
    // e8 59 e8 91 ff           --  call <+ff91e859>
    // 84 c0                    --  test al,al
    // 74 41                    --  je <+41>
    //
    // modded:
    // skip the if-block by jumping always;
    // e8 59 e8 91 ff           --  call <+ff91e859>
    // 84 c0                    --  test al,al
    // eb 41                    --  jmp <+41>
    vanilla = "e8 59 e8 91 ff 84 c0 74 41";
    modded = "e8 59 e8 91 ff 84 c0 eb 41";
    assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    
    Log_Separator();
    Log_Separator();
}

void Apply_ReduceLockOnAngleMod() {
    Log("Apply_ReduceLockOnAngleMod");
    Log_Separator();
    
    // vanilla:
    // initializes the lock-on angle to 0.7f (around 40? degrees); this makes many 
    // enemies lock-on candidates; switching targets just requires moving the mouse
    // rather than aiming at them; this can make things janky and you might switch
    // unintentionally;
    //
    // modded:
    // change this value to 0.25f (around 15? degrees) instead; this affects auto 
    // switching targets when they die; you lose lock-on more often;
    // 0.25f = (0)(011 1110 1)(000 0..) = 3e 80 00 00
    std::string vanilla = "c7 83 2c 29 00 00 c2 b8 32 3f";
    std::string modded = "c7 83 2c 29 00 00 00 00 00 3e";
    uintptr_t assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    
    Log_Separator();
    Log_Separator();
}

void Apply_SwitchLockOnMod() {
    Log("Apply_SwitchLockOnMod");
    Log_Separator();
    
    // vanilla:
    // this function switches locked-on targets; it is called when you move the mouse;
    // 48 89 5c 24 20           --  mov [rsp+20],rbx    <-- return + nops
    // 55                       --  push rbp
    // 56                       --  push rsi
    // 41 57 48 8d 6c 24 90     --  lea rbp,[rsp-70]
    //    
    // modded:
    // focus on the same locked-on target => skip this function by returning 
    // immediately;
    std::string vanilla = "48 89 5c 24 20 55 56 41 57 48 8d 6c 24 90";
    std::string modded = "c3 90 90 90 90 55 56 41 57 48 8d 6c 24 90";
    uintptr_t assembly_location = AobScan(vanilla);
    if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);

    if (target_switching_mode == "modded_switch") {
        Log_Separator();
        
        // vanilla:
        // the score is only used when initiating the lock-on; after that separate switch
        // function(s) are used; every candidate becomes the lowest score after initiation;
        // a8 20                --  test al,20
        // 74 10                --  je <+10>
        // 80 be 30280000 00    --  cmp byte ptr [rsi+00002830],00
        //
        // modded:
        // skip this and leave everyone as a viable candidate; this means that this function
        // now switches lock-on targets as well; the other switch function(s) seem to me more
        // concerned with you moving the mouse rather than aiming directly;
        // a8 20                --  test al,20
        // EB 10                --  jmp <+10>
        // 80 be 30280000 00    --  cmp byte ptr [rsi+00002830],00
        vanilla = "a8 20 74 10 80 be";
        modded = "a8 20 eb 10 80 be";
        assembly_location = AobScan(vanilla);
        if (assembly_location != 0) ReplaceExpectedBytesAtAddress(assembly_location, vanilla, modded);
    }
    
    Log_Separator();
    Log_Separator();
}

//
// main
//

DWORD WINAPI MainThread(LPVOID lpParam) {
    Log("author " + author);
    Log("version " + version);
    
    Log_Separator();
    Log_Separator();
    ReadAndLog_Config();

    if (is_only_using_camera_yaw) Apply_AngleToCameraMod();
    if (camera_height != 1.45F) Apply_CameraHeightMod();
    Apply_FreeLockOnCameraMod();
    Apply_KeepLockOnMod();
    
    // Apply_LockOnCloseRangeMod();
    if (is_health_bar_hidden) Apply_LockOnHealthBarMod();
    Apply_LockOnScoreMod(); // makes LockOnCloseRangeMod useless;
    // Apply_LockOnSensitivityMod();
    
    if (is_toggle) Apply_LockOnToggleMod();
    // Apply_ReduceLockOnAngleMod();
    if (target_switching_mode != "vanilla_switch") Apply_SwitchLockOnMod(); // makes LockOnSensitivityMod useless;
    
    // this can fail when using the original CameraFix mod; in that case it can take 
    // a while and the logs might misalign or get spammed otherwise;
    if (is_toggle) CameraFix::Apply_CameraResetMod();

    CloseLog();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    // someone wrote online that some processes might crash when returning false;
    if (reason != DLL_PROCESS_ATTACH) return true;
    DisableThreadLibraryCalls(module);
    CreateThread(0, 0, &MainThread, 0, 0, NULL);
    return true;
}
