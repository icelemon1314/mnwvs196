#pragma once
#include "PacketFlags.hpp"

#define MAKE_CLIENT_PACKET_FLAG(flagName, value) namespace FlagInstances{ const static ClientPacketFlag flagName {value, #flagName}; } const static int flagName = value

MAKE_FLAG_COLLECTION_BODY(ClientPacketFlag)
//以下開始註冊封包Opcode Flag

MAKE_CLIENT_PACKET_FLAG(ClientCheckPasswordRequest, 0x69);
MAKE_CLIENT_PACKET_FLAG(ClientSelectWorld, 0x6A);
MAKE_CLIENT_PACKET_FLAG(ClientSecondPasswordCheck, 0x6D);
MAKE_CLIENT_PACKET_FLAG(ClientMigrateIn, 0x6E);
MAKE_CLIENT_PACKET_FLAG(ClientSelectCharacter, 0x6F);
MAKE_CLIENT_PACKET_FLAG(ClientRequestServerList, 0x72);
MAKE_CLIENT_PACKET_FLAG(ClientCheckDuplicatedID, 0x74);
MAKE_CLIENT_PACKET_FLAG(ClientCreateNewCharactar, 0x7D);
MAKE_CLIENT_PACKET_FLAG(ClientLoginBackgroundRequest, 0xA0);
MAKE_CLIENT_PACKET_FLAG(ClientRequestStart, 0xA5);


MAKE_CLIENT_PACKET_FLAG(OnUserTransferFieldRequest, 0xB3);
MAKE_CLIENT_PACKET_FLAG(OnUserMoveRequest, 0xC4);
MAKE_CLIENT_PACKET_FLAG(OnUserChat, 0xD1);
MAKE_CLIENT_PACKET_FLAG(OnUserChangeSlotRequest, 0xF7);

//結束Opcode Flag註冊
FLAG_COLLECTION_BODY_END