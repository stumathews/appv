using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace TestClassLibrary
{
    /* Interface for COM exposure */
    public interface TestClassInterface
    {
        string ProcessVirtualChannelRequest(string payload);    
    }
}
