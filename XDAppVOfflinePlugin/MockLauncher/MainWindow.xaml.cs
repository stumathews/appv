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
      // Check to see that the launcher is running on the VDA if not dont try to connect to 
      // a virtual channel as one will not be available if launcher running on endpoint
      // Which will be the case we we're using the launcher to resolve paths in the AppV VFS for utility sake.
      var sessionName = Environment.GetEnvironmentVariable("SESSIONNAME");
      if(sessionName.StartsWith("ICA"))
      {
        try
        {
           var request = new Request() { MessageBody = @"462dfa90-9e09-429e-a6f3-00cb3e9bf90b;\\londondc.london.local\\appvshare\\EPM 3.appv;4148dc52-d7d5-47c3-a570-396aa63fa9fe;d2022911-4251-4c86-b8cd-e2bb092443fd;EPM Opsætningsmodul", RequestTask = RequestTask.AddClientPackage };
           var payload = ServiceBrokerProtocolHelper.Serialize(request);
           var sb = new StringBuilder(payload.Length+1);

           // Call the C++ DLL to do the VirtualChannel Send()
           SendAndRecieve(payload,payload.Length+1, sb, sb.Capacity);

           var response = ServiceBrokerProtocolHelper.Deserialize<Response>(sb.ToString());
           var result = string.Format("{0}:{1}", response.ResponseCode, String.IsNullOrEmpty(response.MessageBody) ? "empty message string" : response.MessageBody);
           Status.Content = result;
           MessageBox.Show("Result was " + result);
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