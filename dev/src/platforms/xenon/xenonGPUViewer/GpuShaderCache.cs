using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace xenonGPUViewer
{
    struct GPUShaderCacheEntry
    {
        public RawMemoryBlock _RawMemory;
        public bool _Pixel;
        public GPUShader _Shader; // may be null
    }

    public class GPUShaderCache
    {
        private List<GPUShaderCacheEntry> _Entries;

        public GPUShaderCache()
        {
            _Entries = new List<GPUShaderCacheEntry>();
        }


        public GPUShader DecompileShader(bool pixel, RawMemoryBlock block)
        {
            // local search
            foreach (var entry in _Entries)
            {
                if (entry._Pixel == pixel && entry._RawMemory == block)
                    return entry._Shader;
            }

            // convert memory
            GPUShader shader = null;
            {
                var words = block.LoadAllDataAs32BE();

                try
                {
                    shader = GPUShader.Decompile(pixel, words);
                }
                catch (Exception)
                {
                }
            }

            // create new entry
            GPUShaderCacheEntry newEntry = new GPUShaderCacheEntry();
            newEntry._Pixel = pixel;
            newEntry._RawMemory = block;
            newEntry._Shader = shader;
            _Entries.Add(newEntry);

            return shader;
        }
    }
}
