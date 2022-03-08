#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250

static struct rte_mempool *mbuf_pool = NULL;

static struct rte_ether_addr ether_src_addr;
static struct rte_ether_addr ether_dst_addr = {{0x00, 0x50, 0x56, 0xc0, 0x00, 0x01}};

static uint32_t ipv4_src_addr = RTE_IPV4(192, 168, 112, 2);
static uint32_t ipv4_dst_addr = RTE_IPV4(192, 168, 112, 1);

static uint16_t udp_src_port = 50000;
static uint16_t udp_dst_port = 50001;

static const struct rte_eth_conf port_conf_default = {
	.rxmode = {
		.max_rx_pkt_len = RTE_ETHER_MAX_LEN,
	},
};

static void 
fill_udp_packet(struct rte_mbuf *pkt, char *data, size_t length)
{
	struct rte_ether_hdr *ether_hdr = 
		rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	
	struct rte_ipv4_hdr *ipv4_hdr =
		rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));

	struct rte_udp_hdr *udp_hdr =
		rte_pktmbuf_mtod_offset(pkt, struct rte_udp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));

	size_t udp_pkt_len = length + sizeof(struct rte_udp_hdr);
	size_t ipv4_pkt_len = udp_pkt_len + sizeof(struct rte_ipv4_hdr);
	size_t ether_pkt_len = ipv4_pkt_len + sizeof(struct rte_ether_hdr);

	pkt->data_len = ether_pkt_len;

	// fill ether header
	ether_hdr->s_addr = ether_src_addr;
	ether_hdr->d_addr = ether_dst_addr;
	ether_hdr->ether_type = rte_be_to_cpu_16(RTE_ETHER_TYPE_IPV4);

	// fill ipv4 header
	ipv4_hdr->version_ihl = RTE_IPV4_VHL_DEF;
	ipv4_hdr->type_of_service = 0;
	ipv4_hdr->total_length = rte_cpu_to_be_16(ipv4_pkt_len);
	ipv4_hdr->packet_id = 0;
	ipv4_hdr->fragment_offset = 0;
	ipv4_hdr->time_to_live = 64;
	ipv4_hdr->next_proto_id = IPPROTO_UDP;
	ipv4_hdr->src_addr = rte_cpu_to_be_32(ipv4_src_addr);
	ipv4_hdr->dst_addr = rte_cpu_to_be_32(ipv4_dst_addr);
	ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);

	// fill udp header
	udp_hdr->src_port = rte_cpu_to_be_16(udp_src_port);
    udp_hdr->dst_port = rte_cpu_to_be_16(udp_dst_port);
    udp_hdr->dgram_len = rte_cpu_to_be_16(udp_pkt_len);
	udp_hdr->dgram_cksum = 0;

	rte_memcpy(rte_pktmbuf_mtod_offset(pkt, void *,
		sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr)),
		data, length);
}

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int
port_init(uint16_t port)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf txconf;

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	retval = rte_eth_dev_info_get(port, &dev_info);
	if (retval != 0) {
		printf("Error during getting device (port %u) info: %s\n",
				port, strerror(-retval));
		return retval;
	}

	if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |=
			DEV_TX_OFFLOAD_MBUF_FAST_FREE;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, nb_txd,
				rte_eth_dev_socket_id(port), &txconf);
		if (retval < 0)
			return retval;
	}

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	struct rte_ether_addr addr;
	retval = rte_eth_macaddr_get(port, &addr);
	if (retval != 0)
		return retval;

	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			port,
			addr.addr_bytes[0], addr.addr_bytes[1],
			addr.addr_bytes[2], addr.addr_bytes[3],
			addr.addr_bytes[4], addr.addr_bytes[5]);

	/* Enable RX in promiscuous mode for the Ethernet device. */
	retval = rte_eth_promiscuous_enable(port);
	if (retval != 0)
		return retval;

	return 0;
}

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
static __rte_noreturn void
lcore_main()
{
	uint16_t port;

	/*
	 * Check that the port is on the same NUMA node as the polling thread
	 * for best performance.
	 */
	RTE_ETH_FOREACH_DEV(port)
		if (rte_eth_dev_socket_id(port) >= 0 &&
				rte_eth_dev_socket_id(port) !=
						(int)rte_socket_id())
			printf("WARNING, port %u is on remote NUMA node to "
					"polling thread.\n\tPerformance will "
					"not be optimal.\n", port);

	printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
			rte_lcore_id());

	rte_eth_macaddr_get(0, &ether_src_addr);
	char *str = "Hello from virtural machine";

	struct rte_mbuf *pkts[1];
    pkts[0] = rte_pktmbuf_alloc(mbuf_pool);
	rte_pktmbuf_reset_headroom(pkts[0]);
    fill_udp_packet(pkts[0], str, strlen(str));

	/* Run until the application is quit or killed. */
	for (;;) {
        const uint16_t nb_tx = rte_eth_tx_burst(0, 0, pkts, 1);
        if (nb_tx) printf("Successfully sent a UDP packet\n");
        else printf("Fail to send a UDP packet\n");
		sleep(1);
	}
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int
main(int argc, char *argv[])
{
	unsigned nb_ports;
	uint16_t portid;

	/* Initialize the Environment Abstraction Layer (EAL). */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	argc -= ret;
	argv += ret;

	/* Check that there is an even number of ports to send/receive on. */
	nb_ports = rte_eth_dev_count_avail();
	if (nb_ports < 1)
		rte_exit(EXIT_FAILURE, "Error: no available ports\n");

	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	/* Initialize all ports. */
	RTE_ETH_FOREACH_DEV(portid)
		if (port_init(portid) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n",
					portid);

	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	/* Call lcore_main on the main core only. */
	lcore_main();

	/* clean up the EAL */
	rte_eal_cleanup();

	return 0;
}
