using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using TestClassLibrary;

namespace MockLauncher
{
  /// <summary>
  /// Interaction logic for MainWindow.xaml
  /// </summary>
  public partial class MainWindow : Window
  {
    public MainWindow()
    {
      InitializeComponent();
      
    }

    [DllImport("VDAAPPV.dll", EntryPoint = "SendAndRecieve", SetLastError = true, CharSet = CharSet.Ansi)]
    private static extern int SendAndRecieve(string message, int messageLength, StringBuilder response, int responseLength);

    private void btnCreateVC_Click( object sender, RoutedEventArgs e )
    {

      // Launch on the VDA starts...
      // if this user is allowed offline access to this app, attempt to send ASD to OfflinePlugin else launch on VDA
      // If send successfull, abort launch on VDA
      // successful result is: 
      // 0) VC Channel established AND
      // 1) App-V client installed on endpoint AND
      // 2) 
      // if send un-successful, continue launch on VDA

      // Check to see that the launcher is running on the VDA if not dont try to connect to 
      // a virtual channel as one will not be available if launcher running on endpoint
      // Which will be the case we we're using the launcher to resolve paths in the AppV VFS for utility sake.
      var sessionName = Environment.GetEnvironmentVariable("SESSIONNAME");
      if(sessionName.StartsWith("ICA"))
      {
        try
        {
            // Sync the Configuration on the VDA with that on the client endpoint before we can launch
            Config config = new Config();
            config.Asds = new string[] { };
            config.Isds = new string[] { };
            var vcSyncRequest = new VirtualChannelRequest { RequestTask = VirtualChannelRequestTask.SyncConfig, MessageBody = ServiceBrokerProtocolHelper.Serialize(config) };
            var vcRawSyncRequest = ServiceBrokerProtocolHelper.Serialize(vcSyncRequest);
            var vcSyncResultRaw = new StringBuilder(512);
            int vcSyncSendResult = SendAndRecieve(vcRawSyncRequest, vcRawSyncRequest.Length+1, vcSyncResultRaw, vcSyncResultRaw.Capacity);
            if(vcSyncSendResult == 2)
            {
              // Could not Sync the config
              return;
            }

            var response = ServiceBrokerProtocolHelper.Deserialize<VirtualChannelResponse>(vcSyncResultRaw.ToString());
            var result = string.Format("{0}:{1}", response.ResponseCode, String.IsNullOrEmpty(response.MessageBody) ? "empty message string" : response.MessageBody);
            Status.Content = result;
            MessageBox.Show("Result was " + result);

            // Now we should arrange for the launch
            // Update isolation groups on the client first if the app is in an isolation group

        }
        catch (Exception ex)
        {
            MessageBox.Show("Error sendreceive: "+ ex.Message);
        }
      }
    }

    private void btnCloseVC_Click( object sender, RoutedEventArgs e )
    {
    }
  }
}