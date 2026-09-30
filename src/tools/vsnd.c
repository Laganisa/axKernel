#include "_macro.h"
#include "global/_debug.h"
#include "global/_io.h"

#include "tools/_virtio.h"

/*
	가상 큐 mmio를 이용한 사운드 코드
*/

#define VIRTIO_SND_QUEUE_COUNT 4
#define VIRTIO_SND_VQ_CONTROL 0
#define VIRTIO_SND_R_PCM_INFO 0x0100
#define VIRTIO_SND_MAX_STREAMS 8
#define SND_DESC_F_NEXT 1
#define SND_DESC_F_WRITE 2

struct virtio_snd_query_info
{
	uint32_t code;
	uint32_t start_id;
	uint32_t count;
	uint32_t size;
} __attribute__((packed));

struct virtio_snd_pcm_info
{
	uint32_t hda_fn_nid;
	uint32_t features;
	uint64_t formats;
	uint64_t rates;
	uint8_t direction;
	uint8_t channels_min;
	uint8_t channels_max;
	uint8_t padding[5];
} __attribute__((packed));

static unsigned char snd_queue_storage[VIRTIO_SND_QUEUE_COUNT][VIRTIO_QUEUE_STORAGE]
	__attribute__((aligned(4096)));

static virtio_queue_state snd_queues[VIRTIO_SND_QUEUE_COUNT];
static uint16_t snd_control_used_idx;

static int snd_control_submit(
	const void *request,
	uint32_t request_size,
	void *response,
	uint32_t response_size,
	uint32_t *response_len)
{
	virtio_queue_state *queue = &snd_queues[VIRTIO_SND_VQ_CONTROL];
	if (queue->size < 2)
	{
		puts("SND control queue too small\n");
		return -1;
	}

	uint16_t avail_idx = queue->avail->idx;
	uint16_t head = (avail_idx * 2) % queue->size;
	uint16_t response_desc = (head + 1) % queue->size;

	queue->desc[head].addr = (uint64_t)(uintptr_t)request;
	queue->desc[head].len = request_size;
	queue->desc[head].flags = SND_DESC_F_NEXT;
	queue->desc[head].next = response_desc;

	queue->desc[response_desc].addr = (uint64_t)(uintptr_t)response;
	queue->desc[response_desc].len = response_size;
	queue->desc[response_desc].flags = SND_DESC_F_WRITE;
	queue->desc[response_desc].next = 0;

	queue->avail->ring[avail_idx % queue->size] = head;
	virtio_mb();
	queue->avail->idx = avail_idx + 1;
	virtio_mb();
	virtio_mmio_notify_queue(g_virtio_snd_base, VIRTIO_SND_VQ_CONTROL);

	uint32_t timeout = 10000000;
	while (queue->used->idx == snd_control_used_idx && timeout-- != 0)
	{
		virtio_mb();
	}

	if (queue->used->idx == snd_control_used_idx)
	{
		puts("SND control request timed out\n");
		return -1;
	}

	struct virtq_used_elem *used =
		&queue->used->ring[snd_control_used_idx % queue->size];
	if (used->id != head)
	{
		dump("SND unexpected control descriptor", used->id);
		return -1;
	}

	*response_len = used->len;
	snd_control_used_idx++;
	return 0;
}

static int snd_query_pcm_info(uint32_t stream_count)
{
	if (stream_count == 0 || stream_count > VIRTIO_SND_MAX_STREAMS)
	{
		dump("SND unsupported stream count", stream_count);
		return -1;
	}

	struct virtio_snd_query_info request = {
		.code = VIRTIO_SND_R_PCM_INFO,
		.start_id = 0,
		.count = stream_count,
		.size = sizeof(struct virtio_snd_pcm_info)};
	struct virtio_snd_pcm_info info[VIRTIO_SND_MAX_STREAMS] = {0};
	uint32_t response_len = 0;

	if (snd_control_submit(
			&request,
			sizeof(request),
			info,
			stream_count * sizeof(info[0]),
			&response_len) < 0)
	{
		return -1;
	}

	uint32_t expected_len = stream_count * sizeof(info[0]);
	dump("SND PCM info response bytes", response_len);
	if (response_len < expected_len)
	{
		puts("SND PCM info response too short\n");
		return -1;
	}

	for (uint32_t i = 0; i < stream_count; ++i)
	{
		dump("SND PCM stream", i);
		dump("SND PCM direction", info[i].direction);
		dump("SND PCM channels min", info[i].channels_min);
		dump("SND PCM channels max", info[i].channels_max);
		dump("SND PCM formats", info[i].formats);
		dump("SND PCM rates", info[i].rates);
	}

	return 0;
}

void vsnd_init(void)
{
	if (g_virtio_snd_base == 0)
	{
		puts("SND MMIO base not found\n");
		return;
	}

	uint32_t version = virtio_mmio_version(g_virtio_snd_base);
	dump("SND MMIO version", version);

	if (version == 1)
	{
		vq_init(g_virtio_snd_base);
	}
	else if (version == 2)
	{
		if (vq_init_v2(g_virtio_snd_base) < 0)
		{
			return;
		}
	}
	else
	{
		puts("Unsupported VirtIO MMIO version\n");
		return;
	}

	for (uint32_t i = 0; i < VIRTIO_SND_QUEUE_COUNT; ++i)
	{
		int result;
		if (version == 1)
		{
			result = vq_setup_v1(
				g_virtio_snd_base,
				(uint16_t)i,
				snd_queue_storage[i],
				&snd_queues[i]);
		}
		else
		{
			result = vq_setup_v2(
				g_virtio_snd_base,
				(uint16_t)i,
				snd_queue_storage[i],
				&snd_queues[i]);
		}

		if (result < 0)
		{
			return;
		}
		dump("SND queue ready", i);
	}

	if (version == 1)
	{
		vq_start_v1(g_virtio_snd_base);
	}
	else
	{
		vq_start_v2(g_virtio_snd_base);
	}

	dump("SND MMIO base", g_virtio_snd_base);
	uint32_t stream_count = virtio_mmio_read_config32(g_virtio_snd_base, 4);
	dump("SND streams", stream_count);
	snd_query_pcm_info(stream_count);
}
