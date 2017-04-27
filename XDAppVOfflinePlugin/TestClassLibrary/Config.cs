using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace TestClassLibrary
{
  [Serializable]
  public class Config
  {
    // ApplicationStartDetails
    public string[] Asds;
    // IsolationGroupStartDetails
    public string[] Isds;
  }
}
