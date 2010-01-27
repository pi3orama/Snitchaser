/* 
 * scan_opcode.c
 */

#include <stdint.h>
#include "x86_opcode.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


extern const struct opcode_table_entry normal_insts[256];
extern const struct opcode_table_entry group_insts[NR_GROUPS][8];

/* if this instruction is jmp, then return NULL. if not, return
 * the address of next instruction */
uint8_t *
next_inst(uint8_t * stream)
{
	uint8_t prefix1 = 0;
	uint8_t prefix2 = 0;
	uint8_t prefix3 = 0;
	uint8_t prefix4 = 0;
	uint8_t opc;
	struct opcode_table_entry inn_e;
	const struct opcode_table_entry * e = NULL;
	uint8_t modrm = 0;

#define MOD(x)	(((x) & 0xc0)>>6)
#define REG(x)	(((x) & 0x38)>>3)
#define RM(x)	(((x) & 0x7))
restart:
	/* lookup in normal table */
	opc = *stream;
	stream ++;
	e = &normal_insts[opc];

group_restart:
	switch (e->type) {
		case INST_PREFIX1:
			prefix1 = opc;
			goto restart;
		case INST_PREFIX2:
			prefix2 = opc;
			goto restart;
		case INST_PREFIX3:
			prefix3 = opc;
			goto restart;
		case INST_PREFIX4:
			prefix4 = opc;
			goto restart;
		case INST_INVALID:
			printf("invalid instruction 0x%x\n", opc);
			exit(-1);
			break;
		case INST_NORMAL:
			break;
		
		case INST_GROUP1_0x80:
		case INST_GROUP1_0x81:
		case INST_GROUP1_0x82:
		case INST_GROUP1_0x83:
		case INST_GROUP1A_0x8f:
		case INST_GROUP2_0xc0:
		case INST_GROUP2_0xc1:
		case INST_GROUP2_0xd0:
		case INST_GROUP2_0xd1:
		case INST_GROUP2_0xd2:
		case INST_GROUP2_0xd3:
		case INST_GROUP3_0xf6:
		case INST_GROUP3_0xf7:
		case INST_GROUP4_0xfe:
		case INST_GROUP5_0xff:
		case INST_GROUP6_0x0f00:
		case INST_GROUP7_0x0f01_mem:
		case INST_GROUP7_0x0f01_11:
		case INST_GROUP8_0x0fba:
		case INST_GROUP9_0x0fc7:
		case INST_GROUP10_0x0fb9:
		case INST_GROUP11_0xc6:
		case INST_GROUP11_0xc7:
		case INST_GROUP12_0x0f71:
		case INST_GROUP13_0x0f72:
		case INST_GROUP14_0x0f73:
		case INST_GROUP15_0x0fae:
		case INST_GROUP16_0x0f18:
			modrm = *stream;
			e = &(group_insts[e->type - INST_GROUP_start - 1]
					[REG(modrm)]);
			break;
		case INST_ESCAPE:
		case INST_SPECIAL:
		default:
			printf("type %d not implelented\n", e->type);
			exit(-1);
	}

	printf("instruction 0x%x:%s\n", opc, e->name);

	if (e->jmpnote != 0)
		return NULL;

	if (e->nr_operades == 0)
		return stream;

	/* modrm */
	int operade_size = 4;
	int address_size = 4;
	/* prefix group 3 */
	if (prefix3 == 0x66)
		operade_size = 2;
	if (prefix4 == 0x67)
		address_size = 2;

	/* modrm and disp */
	if ((e->req_modrm) || 
		((e->type >= INST_GROUP_start) && (e->type <= INST_GROUP_end))) {
		modrm = *stream;
		stream ++;
		/* SIB */
		uint8_t mod_rm = (modrm & 0xc7);
		if (address_size == 2) {
			if (mod_rm == 0x06)
				stream += 2;
			else if (MOD(modrm) == 1)
				stream += 1;
			else if (MOD(modrm) == 2)
				stream += 2;
		} else {
			/* addressing == 4 (*8bit) */
			int disp = 0;
			int have_sib = 0;

			if ((RM(modrm) == 0x04) && (MOD(modrm) != 0x03))
				have_sib = 1;
			if (mod_rm == 0x05)
				disp = 4;
			if (MOD(modrm) == 0x01)
				disp = 1;
			else if (MOD(modrm) == 0x02)
				disp = 4;
			if (have_sib)
				stream += 1;
			stream += disp;
		}
	}

	/* operade */
	if (e->nr_operades == 0)
		return stream;
	for (int i = 0; i < e->nr_operades; i++) {
		int addressing = e->operades[i].addressing;
		switch (addressing) {
			case OP_ADDR_A:
				stream += 2 + operade_size;
				break;
			case OP_ADDR_I:
			case OP_ADDR_J:
				{
					uint32_t sz = e->operades[i].size;
					uint32_t base_sz = sz & 0xffff;
					uint32_t real_sz;
					real_sz = ((sz & RESPECT_OPERADE_SIZE_66) && prefix3) ?
						2 : base_sz;
					stream += real_sz;
				}
				break;
			case OP_ADDR_O:
				stream += address_size;
				break;
			case OP_ADDR_X:
			case OP_ADDR_Y:
				break;
			default:
				if (!((addressing & REQ_MODRM) ||
						(addressing & OPERADE_ADDRESSING_CONSTANT) ||
				   		(addressing & OPERADE_ADDRESSING_REGISTER)))  {
					printf("wrong addressing 0x%x\n", 
							addressing);
					exit(-1);
				}
		}
	}
	return stream;
}

// vim:ts=4:sw=4

