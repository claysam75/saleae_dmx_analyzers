#include "Dmx512Analyzer.h"
#include "Dmx512AnalyzerSettings.h"
#include "MockChannelData.h"
#include "MockResults.h"
#include "MockSimulatedChannelDescriptor.h"
#include "TestInstance.h"

static void Require(bool value, const char* message)
{
    if(!value) throw std::runtime_error(message);
}

static unsigned char Payload(int packet, int slot)
{
    return static_cast<unsigned char>(packet * 19 + slot * 37);
}

// Independent transmitter: absolute sample times, LSB first, 8 data bits, 2 stops.
class Transmitter
{
public:
    Transmitter(AnalyzerTest::MockChannelData& data, U32 sampleRate)
        : data(data), rate(sampleRate), sample(0), state(BIT_HIGH)
    { data.TestSetInitialBitState(BIT_HIGH); }
    void Hold(BitState next, double seconds)
    {
        if(next != state) {
            data.TestAppendTransitionAtSamples(static_cast<U64>(std::llround(sample)));
            state = next;
        }
        sample += seconds * rate;
    }
    void Byte(unsigned char value)
    {
        const double bit = 1.0 / DMX_BIT_RATE;
        Hold(BIT_LOW, bit);
        for(int i=0; i<8; ++i) Hold((value & (1 << i)) ? BIT_HIGH : BIT_LOW, bit);
        Hold(BIT_HIGH, 2 * bit);
    }
private:
    AnalyzerTest::MockChannelData& data;
    U32 rate;
    double sample;
    BitState state;
};

static void CheckDecode(U32 sampleRate, int slots, double mab, double minimumMab, bool expectMabError)
{
    AnalyzerTest::Instance instance(GetAnalyzerName());
    Channel channel(1, 0, DIGITAL_CHANNEL);
    auto* settings = dynamic_cast<Dmx512AnalyzerSettings*>(instance.GetSettings());
    settings->mInputChannel = channel;
    settings->mMinMAB = minimumMab;
    Require(settings->mBitRate == DMX_BIT_RATE, "wrong compiled baud rate");
    instance.SetSampleRate(sampleRate);
    AnalyzerTest::MockChannelData data(&instance);
    Transmitter tx(data, sampleRate);
    tx.Hold(BIT_HIGH, .000100);
    for(int packet=0; packet<2; ++packet) {
        tx.Hold(BIT_LOW, .000100);
        tx.Hold(BIT_HIGH, mab);
        tx.Byte(0);
        for(int slot=1; slot<=slots; ++slot) {
            // Exercise both contiguous slots and inter-slot marks.
            if(slot % 2) tx.Hold(BIT_HIGH, 2.0 / DMX_BIT_RATE);
            tx.Byte(Payload(packet, slot));
        }
        tx.Hold(BIT_HIGH, .000020);
    }
    // Finish with a following BREAK. No falling start edge: the mock has no
    // end-of-capture sample limit and would decode a partial byte as all zeros.
    tx.Hold(BIT_LOW, .000100);
    tx.Hold(BIT_HIGH, mab);
    data.ResetCurrentSample();
    instance.SetChannelData(channel, &data);
    Require(instance.RunAnalyzerWorker() == AnalyzerTest::Instance::WorkerRanOutOfData,
            "worker did not terminate cleanly");
    auto* results = AnalyzerTest::MockResultData::MockFromResults(instance.GetResults());
    int packet=-1, slot=0, mabCount=0;
    for(U64 i=0; i<results->TotalFrameCount(); ++i) {
        const Frame& frame = results->GetFrame(i);
        if(frame.mType == Dmx512Analyzer::MAB) {
            ++mabCount;
            Require(bool(frame.mFlags & DISPLAY_AS_ERROR_FLAG) == expectMabError, "wrong MAB validation");
        } else if(frame.mType == Dmx512Analyzer::START_CODE) {
            if(packet >= 0) Require(slot == slots, "packet truncated");
            ++packet; slot=0;
            Require(frame.mData2 == 0, "wrong start code");
        } else if(frame.mType == Dmx512Analyzer::DATA) {
            ++slot;
            Require(frame.mData1 == static_cast<U64>(slot), "wrong slot index");
            Require(frame.mData2 == Payload(packet, slot), "wrong decoded byte");
            if(slot == slots) {
                instance.GenerateBubbleText(i, channel, Decimal);
                Require(results->TotalStringCount() > 0, "missing slot label");
                const std::string expected = "Slot " + std::to_string(slot) + ":";
                Require(results->GetString(0).find(expected) == 0, "slot label was truncated");
                instance.GenerateTabularText(i, Decimal);
                Require(results->TotalTabularTextCount() > 0, "missing slot table row");
                Require(results->GetTabularText(0).find(expected) == 0, "slot table number was truncated");
            }
        } else if(frame.mType == Dmx512Analyzer::STOP) {
            Require(!(frame.mFlags & DISPLAY_AS_ERROR_FLAG), "bad stop bits");
        }
    }
    Require(packet == 1 && slot == slots, "missing packet or slots");
    Require(mabCount >= 2, "missing MAB");
}

static void CheckSimulation()
{
    AnalyzerTest::Instance instance(GetAnalyzerName());
    Channel channel(1, 0, DIGITAL_CHANNEL);
    auto* settings = dynamic_cast<Dmx512AnalyzerSettings*>(instance.GetSettings());
    settings->mInputChannel = channel;
    instance.RunSimulation(120000, 60000000);
    auto* simulation = instance.GetSimulationChannel(channel);
    Require(simulation != nullptr, "missing simulation");
    simulation->ResetToStart();
    // Simulation starts with a falling edge at sample 0.
    Require(simulation->AdvanceToNextTransition(), "missing BREAK start");
    Require(simulation->GetCurrentState() == BIT_LOW, "BREAK is not low");
    Require(std::fabs(simulation->GetDurationToNextTransition() - .000100) < 1e-7,
            "simulation BREAK changed with baud rate");
    Require(simulation->AdvanceToNextTransition(), "missing MAB start");
    Require(simulation->GetCurrentState() == BIT_HIGH, "MAB is not high");
    Require(std::fabs(simulation->GetDurationToNextTransition() - .000012) < 1e-7,
            "simulation MAB changed with baud rate");
}

int main()
{
    try {
        CheckDecode(60000000, 4096, .000012, .000008, false);
        CheckDecode(48000000, 1024, .000012, .000008, false);
        CheckDecode(60000000, 4, .000004, .000008, true);
        CheckDecode(60000000, 4, .000004, .000004, false);
        CheckSimulation();
        std::cout << DMX_BIT_RATE << " baud: decode, slot counts, fractional sampling, MAB and simulation passed\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
