# GStreamer Kit

A Node.js binding for GStreamer, providing high-level APIs for multimedia streaming and processing.

## Installation

```bash
npm install @gst/kit
```

## Quick Start

### Basic Pipeline

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('videotestsrc ! autovideosink');
pipeline.play();

// Stop after 5 seconds
setTimeout(() => {
  pipeline.stop();
}, 5000);
```

### Working with AppSink (Pull-based Approach)

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('videotestsrc num-buffers=10 ! videoconvert ! appsink name=sink');
const sink = pipeline.getElementByName('sink');

if (sink?.type === 'app-sink-element') {
  pipeline.play();
  
  while (true) {
    const sample = await sink.getSample(); // Explicitly request samples
    if (!sample) break;
    
    console.log('Received frame:', sample.buffer?.length, 'bytes');
  }
  
  pipeline.stop();
}
```

### Working with AppSink (Event-Driven/Push Approach)

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline('videotestsrc num-buffers=10 ! videoconvert ! appsink name=sink');
const sink = pipeline.getElementByName('sink');

if (sink?.type === 'app-sink-element') {
  let frameCount = 0;
  
  // Set up reactive callback - samples are pushed automatically
  const unsubscribe = sink.onSample((sample) => {
    frameCount++;
    console.log(`Frame ${frameCount}:`, sample.buffer?.length, 'bytes');
    
    if (frameCount === 10) {
      unsubscribe();
      pipeline.stop();
    }
  });

  pipeline.play();
}
```

### Extracting Buffer Data with Pad Probes

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline(
  'videotestsrc ! videoconvert ! x264enc ! rtph264pay name=pay ! fakesink'
);
const payloader = pipeline.getElementByName('pay');

if (payloader) {
  // Add probe to capture comprehensive buffer data from pad
  const unsubscribe = payloader.addPadProbe('src', (bufferData) => {
    console.log('Buffer Data:', {
      // Raw buffer data
      buffer: bufferData.buffer, // Buffer object with raw data
      size: bufferData.buffer?.length, // Buffer size in bytes
      
      // Timing information (nanoseconds)
      pts: bufferData.pts,       // Presentation timestamp
      dts: bufferData.dts,       // Decode timestamp
      duration: bufferData.duration,
      
      // Buffer metadata
      flags: bufferData.flags,   // GStreamer buffer flags
      
      // Stream format information
      caps: bufferData.caps,     // Caps object with format details
      
      // RTP data (only for RTP streams)
      rtp: bufferData.rtp ? {
        timestamp: bufferData.rtp.timestamp,
        sequence: bufferData.rtp.sequence,
        ssrc: bufferData.rtp.ssrc,
        payloadType: bufferData.rtp.payloadType
      } : undefined
    });
  });

  pipeline.play();
  
  // ... later, remove the probe
  unsubscribe();
  pipeline.stop();
}
```

### Advanced Example: Multiple Probes with Auto-Cleanup

```javascript
import { Pipeline } from '@gst/kit';

const pipeline = new Pipeline(
  'videotestsrc ! videoconvert ! x264enc name=enc ! rtph264pay name=pay ! fakesink'
);

const encoder = pipeline.getElementByName('enc');
const payloader = pipeline.getElementByName('pay');
const unsubscribes = [];

if (encoder && payloader) {
  // Monitor raw H.264 data from encoder
  const h264Unsub = encoder.addPadProbe('src', (bufferData) => {
    console.log(`H.264: ${bufferData.buffer?.length} bytes, pts=${bufferData.pts}`);
    console.log(`Caps: ${bufferData.caps?.name}`);
  });
  unsubscribes.push(h264Unsub);

  // Monitor RTP data from payloader
  const rtpUnsub = payloader.addPadProbe('src', (bufferData) => {
    if (bufferData.rtp) {
      console.log(`RTP: seq=${bufferData.rtp.sequence}, ts=${bufferData.rtp.timestamp}`);
    }
  });
  unsubscribes.push(rtpUnsub);

  pipeline.play();
  
  // Auto-cleanup when done
  setTimeout(() => {
    unsubscribes.forEach(unsub => unsub());
    pipeline.stop();
  }, 5000);
}
```

## Features

- **Pipeline Management**: Create and control GStreamer pipelines
- **Element Access**: Get and set element properties
- **App Sources & Sinks**: Two approaches for data access:
  - **Pull-based**: Explicitly request samples with `getSample()` (async, controlled timing)
  - **Push-based**: Reactive callbacks with `onSample()` (automatic, real-time)
- **Pad Probes**: Add/remove event-driven callbacks to intercept comprehensive buffer data from any element pad
- **Buffer Analysis**: Extract raw buffer data, timing information, flags, caps, stream metadata, and distinguish between key frames and delta frames
- **RTP Support**: Extract RTP metadata including timestamps, sequence numbers, and more (when available)
- **TypeScript Support**: Full type definitions included

## API Reference

### Pipeline

- `new Pipeline(description: string)`: Create a new pipeline
- `play()`: Start pipeline playback
- `stop()`: Stop pipeline
- `playing()`: Check if pipeline is playing
- `getElementByName(name: string)`: Get element by name

### Element

- `getElementProperty(key: string)`: Get element property
- `setElementProperty(key: string, value: any)`: Set element property
- `addPadProbe(padName: string, callback: Function)`: Add pad probe for comprehensive buffer data including timing, flags, caps, and RTP info (returns unsubscribe function)

### App Elements

- `getSample(timeout?: number)`: Pull sample from appsink on-demand (returns Promise, runs async)
- `onSample(callback: Function)`: Reactive sample reception - samples pushed automatically (returns unsubscribe function)

## Requirements

- **Runtime**: Node.js 16+ or Bun 1.0+ (Deno is not supported)
- **System**: GStreamer 1.14 or higher
- **Build**: CMake 3.15 or higher

## License

MIT
