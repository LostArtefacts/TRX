using System.IO;

namespace TRX_Installer;

public class CueTrack
{
    public const int SectorLength = 2352;

    public CueTrack(string binFilePath, int trackNumber, string mode, string time)
    {
        BinFilePath = binFilePath;
        TrackNumber = trackNumber;
        SetMode(mode);
        StartSector = ToFrames(time);
    }

    public bool Audio { get; private set; }
    public string BinFilePath { get; }
    public int BlockSize { get; private set; }
    public int BlockStart { get; private set; }
    public bool SwapAudioByteOrder { get; private set; }
    public long StartPosition => StartSector * SectorLength;
    public long StartSector { get; }
    public long Stop { get; set; }
    public long StopSector { get; set; }
    public long TotalBytes => (StopSector - StartSector + 1) * BlockSize;
    public int TrackNumber { get; }
    public bool TruncatePsx { get; private set; }
    public bool WavFormat { get; private set; }

    public void Write(string targetPath, IProgress<(int current, int maximum)>? progress)
    {
        using FileStream fileStream = OpenBinFile();
        using Stream stream = File.OpenWrite(targetPath);
        long currentPosition = StartPosition;
        long sector = StartSector;
        long convertedBytes = 0;
        byte[] buffer = new byte[SectorLength];

        while (sector <= StopSector && fileStream.Read(buffer, 0, SectorLength) > 0)
        {
            if (Audio && SwapAudioByteOrder)
            {
                DoByteSwap(buffer);
            }

            stream.Write(buffer, BlockStart, BlockSize);
            currentPosition += SectorLength;
            convertedBytes += BlockSize;

            if (progress is not null && currentPosition / SectorLength % 500 == 0)
            {
                progress.Report(((int)convertedBytes, (int)TotalBytes));
            }

            sector++;
        }

        progress?.Report(((int)TotalBytes, (int)TotalBytes));
    }

    private static long ToFrames(string time)
    {
        string[] segments = time.Split(':');
        int mins = int.Parse(segments[0]);
        int secs = int.Parse(segments[1]);
        int frames = int.Parse(segments[2]);
        return (mins * 60 + secs) * 75 + frames;
    }

    private void DoByteSwap(byte[] buffer)
    {
        int position = BlockStart;
        while (position < BlockSize)
        {
            (buffer[position + 1], buffer[position]) = (buffer[position], buffer[position + 1]);
            position += 2;
        }
    }

    private FileStream OpenBinFile()
    {
        FileStream fileStream = File.OpenRead(BinFilePath);
        fileStream.Seek(StartPosition, SeekOrigin.Begin);
        return fileStream;
    }

    private void SetMode(string mode)
    {
        Audio = false;
        BlockStart = 0;

        switch (mode.ToUpperInvariant())
        {
            case "AUDIO":
                BlockSize = 2352;
                Audio = true;
                break;
            case "MODE1/2352":
                BlockStart = 16;
                BlockSize = 2048;
                break;
            case "MODE2/2336":
                BlockStart = 16;
                BlockSize = 2336;
                break;
            case "MODE2/2352":
                if (TruncatePsx)
                {
                    BlockSize = 2336;
                }
                else
                {
                    BlockStart = 24;
                    BlockSize = 2048;
                }
                break;
            default:
                BlockSize = 2352;
                break;
        }
    }
}
