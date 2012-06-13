/*
 * miaofng@2012 initial version, for lpc178x
 *
 *
 */

#ifndef __PHYLAN_H_
#define __PHYLAN_H_

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define EMAC_PHY_DEFAULT_ADDR		MII_ADDR

/* PHY Basic Registers */
#define PHY_REG_BMCR			0x00 // Basic Mode Control Register
#define PHY_REG_BMSR			0x01 // Basic Mode Status Register

/* PHY Extended Registers */
#define PHY_REG_IDR1			0x02 // PHY Identifier 1
#define PHY_REG_IDR2			0x03 // PHY Identifier 2
#define PHY_REG_ANAR			0x04 // Auto-Negotiation Advertisement
#define PHY_REG_ANLPAR			0x05 // Auto-Neg. Link Partner Abitily
#define PHY_REG_ANER			0x06 // Auto-Neg. Expansion Register

/* PHY Vendor-specific Registers */
#define PHY_REG_SRR			0x10 // Silicon Revision Register
#define PHY_REG_MCSR			0x11 // Mode Control/Status Register
#define PHY_REG_SR			0x12 // Special Register
#define PHY_REG_SECR			0x1A // Symbol Error Conter Register
#define PHY_REG_CSIR			0x1B // Control/Status Indication Reg
#define PHY_REG_SITC			0x1C // Special Internal testability Controls
#define PHY_REG_ISR			0x1D // Interrupt Source Register
#define PHY_REG_IMR			0x1E // Interrupt Mask Register
#define PHY_REG_PHYCSR			0x1F // PHY Special Control/Status Reg

/* PHY Basic Mode Control Register (BMCR) bitmap definitions */
#define PHY_BMCR_RESET_POS		(15) //Reset
#define PHY_BMCR_LOOPBACK_POS		(14) //Loop back
#define PHY_BMCR_SPEEDSEL_POS		(13) //Speed selection
#define PHY_BMCR_AUTONEG_POS		(12) //Auto Negotiation
#define PHY_BMCR_PWRDWN_POS		(11) //Power down mode
#define PHY_BMCR_ISOLATE_POS		(10) //Isolate
#define PHY_BMCR_RESTART_AN_POS		(9) //Restart auto negotiation
#define PHY_BMCR_DUPLEX_POS		(8) //Duplex mode
#define PHY_BMCR_COLLISION_POS		(7) //Collistion test mode

/* PHY Basic Mode Status Status Register (BMSR) bitmap definitions */
#define PHY_BMSR_100BT4_POS		(15) //100Base-T4
#define PHY_BMSR_100BTX_FULL_POS	(14) //100Base-TX Full Duplex
#define PHY_BMSR_100BTX_HALF_POS	(13) //100Base-TX Half Duplex
#define PHY_BMSR_10BT_FULL_POS		(12) //10Base-TX Full Duplex
#define PHY_BMSR_10BT_HALF_POS		(11) //10Base-TX Half Duplex
#define PHY_BMSR_MF_PREAM		(6) //MF Preamable Supress
#define PHY_BMSR_AN_COMPLETE_POS	(5) //Auto-Negotiate Complete
#define PHY_BMSR_REMOTE_FAULT_POS	(4) //Remote Fault
#define PHY_BMSR_AN_ABILITY_POS		(3) //Auto-Negotiate Ability
#define PHY_BMSR_LINK_ESTABLISHED_POS	(2) //Link Status
#define PHY_BMSR_JABBER_DETECT_POS	(1) //Jabber Detect
#define PHY_BMSR_EXTCAPBILITY_POS	(0) //Extended Capabilities


//The Common Registers that are using in all PHY IC with EMAC component of LPC1788
#define EMAC_PHY_REG_BMCR		PHY_REG_BMCR
#define EMAC_PHY_REG_BMSR		PHY_REG_BMSR
#define EMAC_PHY_REG_IDR1		PHY_REG_IDR1
#define EMAC_PHY_REG_IDR2		PHY_REG_IDR2

#define EMAC_PHY_BMCR_RESET		(1 << PHY_BMCR_RESET_POS)
#define EMAC_PHY_BMCR_POWERDOWN		(1 << PHY_BMCR_PWRDWN_POS)
#define EMAC_PHY_BMCR_SPEED_SEL		(1 << PHY_BMCR_SPEEDSEL_POS)
#define EMAC_PHY_BMCR_DUPLEX		(1 << PHY_BMCR_DUPLEX_POS)
#define EMAC_PHY_BMCR_AN		(1 << PHY_BMCR_AUTONEG_POS)


#define EMAC_PHY_BMSR_100BT4		(1 << PHY_BMSR_100BT4_POS)
#define EMAC_PHY_BMSR_100TX_FULL	(1 << PHY_BMSR_100BTX_FULL_POS)
#define EMAC_PHY_BMSR_100TX_HALF	(1 << PHY_BMSR_100BTX_HALF_POS)
#define EMAC_PHY_BMSR_10BT_FULL		(1 << PHY_BMSR_10BT_FULL_POS)
#define EMAC_PHY_BMSR_10BT_HALF		(1 << PHY_BMSR_10BT_HALF_POS)
#define EMAC_PHY_BMSR_MF_PREAM		(1 << PHY_BMSR_MF_PREAM)
#define EMAC_PHY_BMSR_REMOTE_FAULT	(1 << PHY_BMSR_REMOTE_FAULT_POS)
#define EMAC_PHY_BMSR_LINK_ESTABLISHED	(1 << PHY_BMSR_LINK_ESTABLISHED_POS)

#if (CONFIG_PHY_DP83848C == 1)
#define PHY_ID1				(0x2000)
#define PHY_ID2_OUI			(0x0017) //Organizationally Unique Identifer Number
#define PHY_ID2_MANF_MODEL		(0x0009) //Manufacturer Model Number
#define MII_ADDR			(0x0100)
#elif (CONFIG_PHY_LAN8720 == 1)
#define PHY_ID1				(0x0007)
#define PHY_ID2_OUI			(0x0030) //Organizationally Unique Identifer Number
#define PHY_ID2_MANF_MODEL		(0x000F) //Manufacturer Model Number
#define MII_ADDR			(0x0100)
#elif (CONFIG_PHY_KS8721B == 1)
#define PHY_ID1				(0x0022)
#define PHY_ID2_OUI			(0x0005) //Organizationally Unique Identifer Number
#define PHY_ID2_MANF_MODEL		(0x0021) //Manufacturer Model Number
#define MII_ADDR			(0x0100)
#elif (CONFIG_PHY_DM9161A == 1)
#define PHY_ID1				(0x0181)
#define PHY_ID2_OUI			(0x002E) //Organizationally Unique Identifer Number
#define PHY_ID2_MANF_MODEL		(0x000A) //Manufacturer Model Number
#define MII_ADDR			(0x0900)
#else /*default to DP83848C*/
#define PHY_ID1				(0x2000)
#define PHY_ID2_OUI			(0x0017) //Organizationally Unique Identifer Number
#define PHY_ID2_MANF_MODEL		(0x0009) //Manufacturer Model Number
#endif

#define PHY_ID2				(((PHY_ID2_OUI & 0x3F) << 6) | (PHY_ID2_MANF_MODEL & 0x3F))
#define EMAC_PHY_ID1_CRIT		(PHY_ID1)
#define EMAC_PHY_ID2_CRIT		(PHY_ID2)

#ifdef __cplusplus
}
#endif

#endif

