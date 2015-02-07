

#include <net/irda/irda.h>
#include <net/irda/iriap.h>
#include <net/irda/irlmp.h>
#include <net/irda/irttp.h>

#include <net/irda/irlan_provider.h>
#include <net/irda/irlan_event.h>

static int irlan_provider_state_idle(struct irlan_cb *self, IRLAN_EVENT event,
				     struct sk_buff *skb);
static int irlan_provider_state_info(struct irlan_cb *self, IRLAN_EVENT event,
				     struct sk_buff *skb);
static int irlan_provider_state_open(struct irlan_cb *self, IRLAN_EVENT event,
				     struct sk_buff *skb);
static int irlan_provider_state_data(struct irlan_cb *self, IRLAN_EVENT event,
				     struct sk_buff *skb);

static int (*state[])(struct irlan_cb *self, IRLAN_EVENT event,
		      struct sk_buff *skb) =
{
	irlan_provider_state_idle,
	NULL, /* Query */
	NULL, /* Info */
	irlan_provider_state_info,
	NULL, /* Media */
	irlan_provider_state_open,
	NULL, /* Wait */
	NULL, /* Arb */
	irlan_provider_state_data,
	NULL, /* Close */
	NULL, /* Sync */
};

void irlan_do_provider_event(struct irlan_cb *self, IRLAN_EVENT event,
			     struct sk_buff *skb)
{
	IRDA_ASSERT(*state[ self->provider.state] != NULL, return;);

	(*state[self->provider.state]) (self, event, skb);
}

static int irlan_provider_state_idle(struct irlan_cb *self, IRLAN_EVENT event,
				     struct sk_buff *skb)
{
	IRDA_DEBUG(4, "%s()\n", __func__ );

	IRDA_ASSERT(self != NULL, return -1;);

	switch(event) {
	case IRLAN_CONNECT_INDICATION:
	     irlan_provider_connect_response( self, self->provider.tsap_ctrl);
	     irlan_next_provider_state( self, IRLAN_INFO);
	     break;
	default:
		IRDA_DEBUG(4, "%s(), Unknown event %d\n", __func__ , event);
		break;
	}
	if (skb)
		dev_kfree_skb(skb);

	return 0;
}

static int irlan_provider_state_info(struct irlan_cb *self, IRLAN_EVENT event,
				     struct sk_buff *skb)
{
	int ret;

	IRDA_DEBUG(4, "%s()\n", __func__ );

	IRDA_ASSERT(self != NULL, return -1;);

	switch(event) {
	case IRLAN_GET_INFO_CMD:
		/* Be sure to use 802.3 in case of peer mode */
		if (self->provider.access_type == ACCESS_PEER) {
			self->media = MEDIA_802_3;

			/* Check if client has started yet */
			if (self->client.state == IRLAN_IDLE) {
				/* This should get the client going */
				irlmp_discovery_request(8);
			}
		}

		irlan_provider_send_reply(self, CMD_GET_PROVIDER_INFO,
					  RSP_SUCCESS);
		/* Keep state */
		break;
	case IRLAN_GET_MEDIA_CMD:
		irlan_provider_send_reply(self, CMD_GET_MEDIA_CHAR,
					  RSP_SUCCESS);
		/* Keep state */
		break;
	case IRLAN_OPEN_DATA_CMD:
		ret = irlan_parse_open_data_cmd(self, skb);
		if (self->provider.access_type == ACCESS_PEER) {
			/* FIXME: make use of random functions! */
			self->provider.send_arb_val = (jiffies & 0xffff);
		}
		irlan_provider_send_reply(self, CMD_OPEN_DATA_CHANNEL, ret);

		if (ret == RSP_SUCCESS) {
			irlan_next_provider_state(self, IRLAN_OPEN);

			/* Signal client that we are now open */
			irlan_do_client_event(self, IRLAN_PROVIDER_SIGNAL, NULL);
		}
		break;
	case IRLAN_LMP_DISCONNECT:  /* FALLTHROUGH */
	case IRLAN_LAP_DISCONNECT:
		irlan_next_provider_state(self, IRLAN_IDLE);
		break;
	default:
		IRDA_DEBUG( 0, "%s(), Unknown event %d\n", __func__ , event);
		break;
	}
	if (skb)
		dev_kfree_skb(skb);

	return 0;
}

static int irlan_provider_state_open(struct irlan_cb *self, IRLAN_EVENT event,
				     struct sk_buff *skb)
{
	IRDA_DEBUG(4, "%s()\n", __func__ );

	IRDA_ASSERT(self != NULL, return -1;);

	switch(event) {
	case IRLAN_FILTER_CONFIG_CMD:
		irlan_provider_parse_command(self, CMD_FILTER_OPERATION, skb);
		irlan_provider_send_reply(self, CMD_FILTER_OPERATION,
					  RSP_SUCCESS);
		/* Keep state */
		break;
	case IRLAN_DATA_CONNECT_INDICATION:
		irlan_next_provider_state(self, IRLAN_DATA);
		irlan_provider_connect_response(self, self->tsap_data);
		break;
	case IRLAN_LMP_DISCONNECT:  /* FALLTHROUGH */
	case IRLAN_LAP_DISCONNECT:
		irlan_next_provider_state(self, IRLAN_IDLE);
		break;
	default:
		IRDA_DEBUG(2, "%s(), Unknown event %d\n", __func__ , event);
		break;
	}
	if (skb)
		dev_kfree_skb(skb);

	return 0;
}

static int irlan_provider_state_data(struct irlan_cb *self, IRLAN_EVENT event,
				     struct sk_buff *skb)
{
	IRDA_DEBUG(4, "%s()\n", __func__ );

	IRDA_ASSERT(self != NULL, return -1;);
	IRDA_ASSERT(self->magic == IRLAN_MAGIC, return -1;);

	switch(event) {
	case IRLAN_FILTER_CONFIG_CMD:
		irlan_provider_parse_command(self, CMD_FILTER_OPERATION, skb);
		irlan_provider_send_reply(self, CMD_FILTER_OPERATION,
					  RSP_SUCCESS);
		break;
	case IRLAN_LMP_DISCONNECT: /* FALLTHROUGH */
	case IRLAN_LAP_DISCONNECT:
		irlan_next_provider_state(self, IRLAN_IDLE);
		break;
	default:
		IRDA_DEBUG( 0, "%s(), Unknown event %d\n", __func__ , event);
		break;
	}
	if (skb)
		dev_kfree_skb(skb);

	return 0;
}










