
/*                                                                          */
/*  Module Name : cagg.h                                                    */
/*                                                                          */
/*  Abstract                                                                */
/*      This module contains A-MPDU aggregation relatived functions.        */
/*                                                                          */
/*  NOTES                                                                   */
/*      None                                                                */
/*                                                                          */
/****************************************************************************/
/*Revision History:                                                         */
/*    Who         When        What                                          */
/*    --------    --------    ----------------------------------------------*/
/*                                                                          */
/*    Honda       12-4-06     created                                       */
/*                                                                          */
/****************************************************************************/

#ifndef _CAGG_H
#define _CAGG_H



#define ZM_AGG_FPGA_DEBUG                   1
#define ZM_AGG_FPGA_REORDERING              1

#ifndef ZM_AGG_TALLY
//#define ZM_AGG_TALLY
#endif


#define ZM_AGG_POOL_SIZE                    20
#define ZM_BAW_POOL_SIZE                    32
#define ZM_AGGQ_SIZE                        64
#define ZM_AGGQ_SIZE_MASK                   (ZM_AGGQ_SIZE-1)
#define ZM_AGG_LOW_THRESHOLD                1
#define ZM_AGG_HIGH_THRESHOLD               5

#define ZM_AC                               4
#define ZM_AGG_CLEAR_TIME                   10
#define ZM_AGG_DELETE_TIME                  10000

#define ZM_AGG_BAW_SIZE                     64
#define ZM_AGG_BAW_MASK                     (ZM_AGG_BAW_SIZE-1)
#define ZM_AGG_ADDBA_REQUEST                1
#define ZM_AGG_ADDBA_REQUEST_ACK            2
#define ZM_AGG_ADDBA_RESPONSE               3
#define ZM_AGG_ADDBA_RESPONSE_ACK           4

#define ZM_AGG_SINGLE_MPDU                  00
#define ZM_AGG_FIRST_MPDU                   01
#define ZM_AGG_MIDDLE_MPDU                  11
#define ZM_AGG_LAST_MPDU                    10

#define TID_TX  struct aggQueue*
#define TID_BAW struct baw_q*
#define BAW wd->baw_enabler
#define DESTQ wd->destQ

#define zm_agg_qlen(dev, head, tail) ((head - tail) & ZM_AGGQ_SIZE_MASK)
#define zm_agg_inQ(tid_tx, pt) ((((pt - tid_tx->aggTail) & ZM_AGGQ_SIZE_MASK) < \
        ((tid_tx->aggHead - tid_tx->aggTail) & ZM_AGGQ_SIZE_MASK))? TRUE:FALSE)
#define zm_agg_plus(pt) pt = (pt + 1) & ZM_AGGQ_SIZE_MASK
#define zm_agg_min(A, B) ((A>B)? B:A)
#define zm_agg_GetTime() wd->tick
#define TXQL (zfHpGetMaxTxdCount(dev) - zfHpGetFreeTxdCount(dev))

/* don't change AGG_MIN_TXQL easily, this might cause BAW BSOD */
#define AGG_MIN_TXQL                        2
#define zm_agg_dynamic_threshold(dev, ar)   ((ar > 16)? 11: \
                                             (ar > 12)? 8: \
                                             (ar > 8)? 5: \
                                             (ar > 4)? 2:1)
#define zm_agg_weight(ac)   ((3 == ac)? 4: \
                             (2 == ac)? 3: \
                             (0 == ac)? 2:1)

#define zm_agg_ratio(ac)    ((3 == ac)? 3: \
                             (2 == ac)? (zfHpGetMaxTxdCount(dev)*1/4): \
                             (0 == ac)? (zfHpGetMaxTxdCount(dev)*2/4): \
                                        (zfHpGetMaxTxdCount(dev)*3/4))

//#define zm_agg_ratio(ac)    3

#define ZM_AGGMSG_LEV    ZM_LV_3
#define zm_msg0_agg(lv, msg) if (ZM_AGGMSG_LEV >= lv) \
        {zm_debug_msg0(msg);}
#define zm_msg1_agg(lv, msg, val) if (ZM_AGGMSG_LEV >= lv) \
        {zm_debug_msg1(msg, val);}
#define zm_msg2_agg(lv, msg, val) if (ZM_AGGMSG_LEV >= lv) \
        {zm_debug_msg2(msg, val);}

#ifndef ZM_ENABLE_FW_BA_RETRANSMISSION //disable BAW
struct baw_header_r {
    u16_t       *header;
    u16_t       *mic;
    u16_t       *snap;
    u16_t       headerLen;
    u16_t       micLen;
    u16_t       snapLen;
    u16_t       removeLen;
    u8_t        keyIdx;
};

struct baw_header {
    u16_t       header[29];//[(8+30+2+18)/2];  58 bytes  /* ctr+(4+a1+a2+a3+2+a4)+qos+iv */
    u16_t       headerLen;
    u16_t       mic[4]; //[8/2]; 8 bytes
    u16_t       micLen;
    u16_t       snap[4]; //[8/2]; 8 bytes
    u16_t       snapLen;
    u16_t       removeLen;
    u8_t        keyIdx;
};

struct bufInfo {
    zbuf_t*     buf;
    u8_t        baw_retransmit;
    u32_t       timestamp;
    struct baw_header   *baw_header;
};
#endif
struct aggElement
{
    zbuf_t*     buf;
    u32_t       arrivalTime;
    u8_t        baw_retransmit;
    struct zsAdditionInfo addInfo;
    //struct baw_header  baw_header;
};


#ifndef ZM_ENABLE_FW_BA_RETRANSMISSION //disable BAW
struct baw_buf
{
    zbuf_t*     buf;
    u16_t       baw_seq;
    u32_t       timestamp;
    u8_t        baw_retransmit;
    struct baw_header baw_header;
};

struct baw_q {
    struct baw_buf  frame[ZM_VTXQ_SIZE];
    u16_t       enabled;
    u16_t       start_seq;
    u16_t       head;
    u16_t       tail;
    u16_t       size;
    TID_TX      tid_tx;

    //struct baw_header *baw_header;
};

struct baw_enabler
{
    struct baw_q    tid_baw[ZM_BAW_POOL_SIZE];
    u8_t    delPoint;
    void    (*core)(zdev_t* dev, u16_t baw_seq, u32_t bitmap, u16_t aggLen);
    //void    (*core);
    void    (*init)(zdev_t* dev);
    TID_BAW (*getNewQ)(zdev_t* dev, u16_t start_seq, TID_TX tid_tx);
    TID_BAW (*getQ)(zdev_t* dev, u16_t baw_seq);
    u16_t   (*insert)(zdev_t* dev, zbuf_t* buf, u16_t baw_seq, TID_BAW tid_baw, u8_t baw_retransmit, struct baw_header_r *header_r);
    struct bufInfo* (*pop)(zdev_t* dev, u16_t index, TID_BAW tid_baw);
    void    (*enable)(zdev_t* dev, TID_BAW tid_baw, u16_t start_seq);
    void    (*disable)(zdev_t* dev, TID_BAW tid_baw);

};
#endif
struct aggQueue
{
    struct      aggElement  aggvtxq[ZM_AGGQ_SIZE];
    u16_t       aggHead;
    u16_t       aggTail;
    s16_t       size;
    u16_t       aggQSTA;
    u16_t       aggQEnabled;
    u16_t       ac;
    u16_t       tid;
    u16_t       aggReady;
    u16_t       clearFlag;
    u16_t       deleteFlag;
    u32_t       lastArrival;
    u16_t       aggFrameSize;
    u16_t       bar_ssn;    /* starting sequence number in BAR */
    u16_t       dst[3];
    u16_t       complete;     /* complete indication pointer */
};

struct aggSta
{
    u16_t       count[ZM_AC];
    TID_TX      tid_tx[8];
    u16_t       aggFlag[ZM_AC];
};

struct agg_tid_rx
{
    u16_t       aid;
    u16_t       ac;
    u16_t       addBaExchangeStatusCode;
    //struct zsAdditionInfo *addInfo;
    u16_t       seq_start;		/* first seq expected next */
    u16_t       baw_head;		/* head of valid block ack window */
    u16_t       baw_tail;		/* tail of valid block ack window */
    //u16_t       free_count;		/* block ack window size	*/
    u8_t        sq_exceed_count;
    u8_t        sq_behind_count;
    struct aggElement frame[ZM_AGG_BAW_SIZE + 1]; /* out-of-order rx frames */
};

struct aggControl
{
    u16_t       aggEnabled;
    u16_t       ampduIndication;
    u16_t       addbaIndication;
    //TID_BAW     tid_baw;
    u32_t       timestamp;
};

struct aggBaFrameParameter
{
    zbuf_t*     buf;
    u16_t       ba_parameter;
    u8_t        dialog;
    u16_t       ba_policy;
    u16_t       tid;
    u16_t       buffer_size;
    u16_t       ba_timeout;
    u16_t       ba_start_seq;
    u16_t       status_code;
};

struct aggBarControl
{
    u16_t       bar_ack_policy      ;
    u16_t       multi_tid           ;
    u16_t       compressed_bitmap   ;
    u16_t       tid_info            ;
};

struct aggTally
{
    u32_t       got_packets_sum;
    u32_t       got_bytes_sum;
    u32_t       sent_packets_sum;
    u32_t       sent_bytes_sum;
    u32_t       avg_got_packets;
    u32_t       avg_got_bytes;
    u32_t       avg_sent_packets;
    u32_t       avg_sent_bytes;
    u16_t       time;
};


struct destQ {
    struct dest{
        u16_t   Qtype : 1; /* 0 aggr, 1 vtxq */
        TID_TX  tid_tx;
        void*   vtxq;

        struct dest* next;
    } *dest[4];
    struct dest* Head[4];
    //s16_t   size[4];
    u16_t   ppri;
    void    (*insert)(zdev_t* dev, u16_t Qtype, u16_t ac, TID_TX tid_tx, void* vtxq);
    void    (*delete)(zdev_t* dev, u16_t Qtype, TID_TX tid_tx, void* vtxq);
    void    (*init)(zdev_t* dev);
    struct dest* (*getNext)(zdev_t* dev, u16_t ac);
    u16_t   (*exist)(zdev_t* dev, u16_t Qtype, u16_t ac, TID_TX tid_tx, void* vtxq);
    //void    (*scan)(zdev_t* dev);
};
void    zfAggInit(zdev_t* dev);
u16_t   zfApFindSta(zdev_t* dev, u16_t* addr);
u16_t   zfAggGetSta(zdev_t* dev, zbuf_t* buf);
TID_TX  zfAggTxGetQueue(zdev_t* dev, u16_t aid, u16_t tid);
TID_TX  zfAggTxNewQueue(zdev_t* dev, u16_t aid, u16_t tid, zbuf_t* buf);
u16_t   zfAggTxEnqueue(zdev_t* dev, zbuf_t* buf, u16_t aid, TID_TX tid_tx);
u16_t   zfAggTx(zdev_t* dev, zbuf_t* buf, u16_t tid);
u16_t   zfAggTxReadyCount(zdev_t* dev, u16_t ac);
u16_t   zfAggTxPartial(zdev_t* dev, u16_t ac, u16_t readycount);
u16_t   zfAggTxSend(zdev_t* dev, u32_t freeTxd, TID_TX tid_tx);
TID_TX  zfAggTxGetReadyQueue(zdev_t* dev, u16_t ac);
zbuf_t* zfAggTxGetVtxq(zdev_t* dev, TID_TX tid_tx);
u16_t   zfAggTxDeleteQueue(zdev_t* dev, u16_t qnum);
u16_t   zfAggScanAndClear(zdev_t* dev, u32_t time);
u16_t   zfAggClearQueue(zdev_t* dev);
void    zfAggTxScheduler(zdev_t* dev, u8_t ScanAndClear);

/* tid_tx manipulation */
#ifndef ZM_ENABLE_FW_BA_RETRANSMISSION //disable BAW
u16_t   zfAggTidTxInsertHead(zdev_t* dev, struct bufInfo* buf_info, TID_TX tid_tx);
#endif
void    zfAggDestInsert(zdev_t* dev, u16_t Qtype, u16_t ac, TID_TX tid_tx, void* vtxq);
void    zfAggDestDelete(zdev_t* dev, u16_t Qtype, TID_TX tid_tx, void* vtxq);
void    zfAggDestInit(zdev_t* dev);
struct dest* zfAggDestGetNext(zdev_t* dev, u16_t ac);
u16_t   zfAggDestExist(zdev_t* dev, u16_t Qtype, u16_t ac, TID_TX tid_tx, void* vtxq);
struct agg_tid_rx *zfAggRxEnabled(zdev_t* dev, zbuf_t* buf);
u16_t   zfAggRx(zdev_t* dev, zbuf_t* buf, struct zsAdditionInfo *addInfo, struct agg_tid_rx *tid_rx);
struct agg_tid_rx *zfAggRxGetQueue(zdev_t* dev, zbuf_t* buf);
u16_t   zfAggRxEnqueue(zdev_t* dev, zbuf_t* buf, struct agg_tid_rx *tid_rx, struct zsAdditionInfo *addInfo);
u16_t   zfAggRxFlush(zdev_t* dev, u16_t seq_no, struct agg_tid_rx *tid_rx);
u16_t   zfAggRxFreeBuf(zdev_t* dev, u16_t destroy);
u16_t   zfAggRxClear(zdev_t* dev, u32_t time);
void    zfAggRecvBAR(zdev_t* dev, zbuf_t* buf);

u16_t   zfAggSendAddbaRequest(zdev_t* dev, u16_t *dst, u16_t ac, u16_t up);
u16_t   zfAggSetAddbaFrameBody(zdev_t* dev,zbuf_t* buf, u16_t offset, u16_t ac, u16_t up);
u16_t   zfAggGenAddbaHeader(zdev_t* dev, u16_t* dst,
                u16_t* header, u16_t len, zbuf_t* buf, u16_t vap, u8_t encrypt);
u16_t   zfAggProcessAction(zdev_t* dev, zbuf_t* buf);
u16_t   zfAggBlockAckActionFrame(zdev_t* dev, zbuf_t* buf);
u16_t   zfAggRecvAddbaRequest(zdev_t* dev, zbuf_t* buf);
u16_t   zfAggRecvAddbaResponse(zdev_t* dev, zbuf_t* buf);
u16_t   zfAggRecvDelba(zdev_t* dev, zbuf_t* buf);
u16_t   zfAggSendAddbaResponse(zdev_t* dev, struct aggBaFrameParameter *bf);
u16_t   zfAggSetAddbaResponseFrameBody(zdev_t* dev, zbuf_t* buf,
                struct aggBaFrameParameter *bf, u16_t offset);
u16_t   zfAggAddbaSetTidRx(zdev_t* dev, zbuf_t* buf,
                struct aggBaFrameParameter *bf);
u16_t zfAggTxSendEth(zdev_t* dev, zbuf_t* buf, u16_t port, u16_t bufType, u8_t flag, struct aggControl *aggControl, TID_TX tid_tx);

u16_t zfAggTallyReset(zdev_t* dev);

u16_t   zfAggPrintTally(zdev_t* dev);

void    zfAggInvokeBar(zdev_t* dev, TID_TX tid_tx);
u16_t   zfAggSendBar(zdev_t* dev, TID_TX tid_tx, struct aggBarControl *aggBarControl);
u16_t   zfAggSetBarBody(zdev_t* dev, zbuf_t* buf, u16_t offset, TID_TX tid_tx, struct aggBarControl *aggBarControl);
u16_t   zfAggGenBarHeader(zdev_t* dev, u16_t* dst,
                u16_t* header, u16_t len, zbuf_t* buf, u16_t vap, u8_t encrypt);

#ifndef ZM_ENABLE_FW_BA_RETRANSMISSION //disable BAW
/* BAW BA retransmission */
void    zfBawCore(zdev_t* dev, u16_t baw_seq, u32_t bitmap, u16_t aggLen);
void    zfBawInit(zdev_t* dev);
TID_BAW zfBawGetNewQ(zdev_t* dev, u16_t start_seq, TID_TX tid_tx);
u16_t   zfBawInsert(zdev_t* dev, zbuf_t* buf, u16_t baw_seq, TID_BAW tid_baw, u8_t baw_retransmit, struct baw_header_r *header_r);
struct bufInfo* zfBawPop(zdev_t* dev, u16_t index, TID_BAW tid_baw);
void    zfBawEnable(zdev_t* dev, TID_BAW tid_baw, u16_t start_seq);
void    zfBawDisable(zdev_t* dev, TID_BAW tid_baw);
TID_BAW zfBawGetQ(zdev_t* dev, u16_t baw_seq);
void zfAggTxRetransmit(zdev_t* dev, struct bufInfo *buf_info, struct aggControl *aggControl, TID_TX tid_tx);
#endif
/* extern functions */
extern zbuf_t* zfGetVtxq(zdev_t* dev, u8_t ac);

#endif /* #ifndef _CAGG_H */

