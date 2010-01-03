#ifndef _SERVER_PCAP_PORT_H
#define _SERVER_PCAP_PORT_H

#include <QTemporaryFile>
#include <QThread>
#include <pcap.h>

#include "abstractport.h"
#include "pcapextra.h"

class PcapPort : public AbstractPort
{
public:
    PcapPort(int id, const char *device);
    ~PcapPort();

    void init();

    virtual void clearPacketList() { 
        transmitter_->clearPacketList();
        setPacketListLoopMode(false);
    }
    virtual bool appendToPacketList(long sec, long usec, const uchar *packet, 
            int length) {
        return transmitter_->appendToPacketList(sec, usec, packet, length); 
    }
    virtual void setPacketListLoopMode(bool loop) {
        transmitter_->setPacketListLoopMode(loop);
    }

    virtual void startTransmit() { 
        if (isDirty())
            updatePacketList();
        transmitter_->start(); 
    }
    virtual void stopTransmit()  { transmitter_->stop();  }
    virtual bool isTransmitOn() { return transmitter_->isRunning(); }

    virtual void startCapture() { capturer_->start(); }
    virtual void stopCapture()  { capturer_->stop(); }
    virtual bool isCaptureOn()  { return capturer_->isRunning(); }
    virtual QIODevice* captureData() { return capturer_->captureFile(); }

protected:
    enum Direction
    {
        kDirectionRx,
        kDirectionTx
    };

    class PortMonitor: public QThread
    {
    public:
        PortMonitor(const char *device, Direction direction,
                AbstractPort::PortStats *stats);
        void run();
        pcap_t* handle() { return handle_; }
        Direction direction() { return direction_; }
        bool isDirectional() { return isDirectional_; }
    protected:
        AbstractPort::PortStats *stats_;
    private:
        pcap_t *handle_;
        Direction direction_;
        bool isDirectional_;
    };

    class PortTransmitter: public QThread
    {
    public:
        PortTransmitter(const char *device);
        void clearPacketList();
        bool appendToPacketList(long sec, long usec, const uchar *packet, 
            int length);
        void setPacketListLoopMode(bool loop) {
            returnToQIdx_ = loop ? 0 : -1;
        }
        void setHandle(pcap_t *handle);
        void useExternalStats(AbstractPort::PortStats *stats);
        void run();
        void stop();
    private:
        int sendQueueTransmit(pcap_t *p, pcap_send_queue *queue, int sync);

        QList<pcap_send_queue*> sendQueueList_;
        int returnToQIdx_;
        bool usingInternalStats_;
        AbstractPort::PortStats *stats_;
        bool usingInternalHandle_;
        pcap_t *handle_;
        bool stop_;
    };

    class PortCapturer: public QThread
    {
    public:
        PortCapturer(const char *device);
        ~PortCapturer();
        void run();
        void stop();
        QFile* captureFile();

    private:
        QString         device_;
        bool            stop_;
        QTemporaryFile  capFile_;
        pcap_t          *handle_;
        pcap_dumper_t   *dumpHandle_;
    };

    PortMonitor     *monitorRx_;
    PortMonitor     *monitorTx_;
private:
    PortTransmitter *transmitter_;
    PortCapturer    *capturer_;

    static pcap_if_t        *deviceList_;
};

#endif