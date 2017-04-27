using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics.CodeAnalysis;
using Microsoft.Win32;

namespace TestClassLibrary
{
    public enum AppVVersionCheckCriteria
    {
        OnlyMajor   = 0,    // Check only the major version
        Exact       = 1,    // Check exact version of App-V
        LesserThan  = 2,    // Check if installed App-V lesser than provided version
        GreaterThan = 3     // Check if installed App-V version greater than provided version
    }
    public static class AppVClientInfo
    {
        private const string m_appv5RegistryLocation = @"Software\Microsoft\AppV\Client";

        [SuppressMessageAttribute("Microsoft.Design", "CA1031:DoNotCatchGeneralExceptionTypes", Justification = "Needed for graceful termination.")]
        public static bool IsAppVClientPresent(Version versionRequired, AppVVersionCheckCriteria versionCheckCriteria)
        {            

            bool isClientPresent = false;
            string strVerRequired = versionRequired.ToString(4);

            RegistryKey registryKey = Registry.LocalMachine.OpenSubKey(m_appv5RegistryLocation);
            if (null != registryKey)
            {
                string versionInstalled = (string)registryKey.GetValue("version");
                if (!string.IsNullOrEmpty(versionInstalled))
                {
                    Version verInstalled = new Version(versionInstalled);
                            
                    switch (versionCheckCriteria)
                    {
                        case AppVVersionCheckCriteria.OnlyMajor:
                            /* This is the list of Supported App-V Clients, and just by checking 
                            * the Major Version we decide that anything above 5.0.0.0 is supported
                            * 
                            * Scenarios            Installed App-V Version         SUPPORTED
                            *      1.                   5.a.b.c                       YES
                            *      2.                   6.x.y.z                       YES
                            *      3.                  10.p.q.r                       YES
                            *      4.                   4.a.b.c                        NO 
                            */
                            isClientPresent = verInstalled.Major >= versionRequired.Major;
                            break;

                        case AppVVersionCheckCriteria.Exact:
                            /*
                            * This part of the code "ELSE block won't be ever hit, because now we only check for a Major Version of Installed client 
                            * to be greater than 5. We won't be checking the Minor/Patch/Build Versions anymore, since our requirement is clear -
                            * SUPPORT ANY APP-V Client GREATER THAN 5.0.0.0
                            */
                            isClientPresent = (verInstalled == versionRequired);
                            break;

                        case AppVVersionCheckCriteria.GreaterThan:
                            /* Check if installed version > supplied version. Used for functionalities present in higher version such as SP2 but not 
                            * in SP1. One such functionality is -HideUI switch
                            */
                            isClientPresent = (verInstalled > versionRequired);
                            break;

                        case AppVVersionCheckCriteria.LesserThan:
                            isClientPresent = (verInstalled < versionRequired);
                            break;
                    }
                }
                else
                {
                    /*Logger.CdfLogger.TraceMsg5("App-V Client registry key found, but version value not found. Setting the client state to absent.", Globals.ModuleName);
                    FileLogger.TraceMsg("App-V Client registry key found, but version value not found. Setting the client state to absent.", Globals.ModuleName);*/
                }

                registryKey.Close();
            }
            else
            {
                /*Logger.CdfLogger.TraceMsg5("Failed to open registry key.");
                FileLogger.TraceMsg("Failed to open registry key.", Globals.ModuleName);*/
            }

            

            return isClientPresent;
        }
    }
}
