import { Pipeline } from ".";

/**
 * Check if a GStreamer element/plugin is available
 */
export const isPluginAvailable = (elementName: string): boolean => {
  try {
    // Try to create a simple pipeline with the element
    new Pipeline(`videotestsrc num-buffers=1 ! ${elementName} ! fakesink`);
    return true;
  } catch (error: unknown) {
    return !(
      error instanceof Error &&
      (error.message.includes("no element") || error.message.includes("no such element"))
    );
  }
};

export const arePluginsAvailable = (plugins: string[]) =>
  plugins.every(plugin => isPluginAvailable(plugin));

export const isWindows = process.platform === "win32";
