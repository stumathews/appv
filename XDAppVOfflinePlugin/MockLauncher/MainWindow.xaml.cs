using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Navigation;
using System.Windows.Shapes;

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
        try
        {
           var sb = new StringBuilder(100);
           SendAndRecieve(textBlock1.Text,textBlock1.Text.Length+1, sb, sb.Capacity);

           Status.Content = sb.ToString();
           MessageBox.Show("Result was " + sb);
        }
        catch (Exception ex)
        {
            MessageBox.Show("Error sendreceive: "+ ex.Message);
        }
    }

    private void btnCloseVC_Click( object sender, RoutedEventArgs e )
    {
    }
  }
}