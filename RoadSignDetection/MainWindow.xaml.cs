using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.ExceptionServices;
using System.Security;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Excel = Microsoft.Office.Interop.Excel;
using System.ComponentModel;
using MessageBox = System.Windows.MessageBox;
using System.Management;
//using Syncfusion.Windows.Shared;


namespace VehicleCounter
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        //public static string user = "";

        private string app_result = "";
        //private int lane_width = 0;

        Excel.Application xlApp;
        Excel.Workbook xlWorkBook;
        Excel.Worksheet xlWorkSheet;
        object misValue;
        string root_dir;
        public static int xls_lastrow_idx = 2;
        //int GlobalObjectCounter;

        object misvalue = System.Reflection.Missing.Value;
        string camera_name = "";

        public MainWindow()
        {
            InitializeComponent();
            root_dir = System.IO.Path.GetDirectoryName(System.Windows.Forms.Application.ExecutablePath);

            //create_new_excel();
            Engine = new EngineSharp.VehicleCounter();
            Engine.ReceivedCameraFrame += Engine_ReceivedCameraFrame;

            //MainWindow.user = "hello";
            //txtlane_result.Text  = to_string(1);
        }

       
        private void Engine_ReceivedCameraFrame(object sender, string result)
        {
            try
            {
                using (Bitmap bitmap = Engine.GetFrame())
                {
                    App.Current.Dispatcher.Invoke(() =>
                    {
                        app_result = result;
                        //result2CSV(app_result);
                        imgPreview.Source = Bitmap2BitmapImage(bitmap);
                    });
                }
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show("Received - " + ex.Message);
            }
        }

        private void result2CSV(string feedback_result)
        {
            string[] several_line_text = feedback_result.Split('\n');
            string lane_result = several_line_text[0];
            string[] lane_Value = lane_result.Split(':');

            //imgPreview_2.Source = imgPreview.Source;
            lane_width = Convert.ToDouble(lane_Value[1]) * lane_width_factor;
            lane_width = Math.Round(lane_width, 2);

            if (lane_width>0)
            {
                txtlane_result.Text = "";
                txtlane_result.Text = "LW : " + lane_width.ToString() + " meter";
            }          

            try
            {
                TextWriter txt = new StreamWriter("C:\\NSV, GEOTRAN\\LAS\\las parameters.ini");
                txt.Write(lane_width_factor.ToString() + "\n");
                txt.Write(lane_width.ToString() + "\n");
                txt.Write(camera_name + "\n");               
                txt.Close();
            }
            catch (Exception )
            {

            }

            string []signboard_results = several_line_text[1].Split(':');

            string road_class = "";
            int left_pos = 0;
            int right_pos = 0;
            int width = 0;
            int height = 0;
            int distance = 0;


            if (signboard_results.Length == 5)
            {
                road_class = signboard_results[0];
                left_pos = System.Convert.ToInt32(signboard_results[1]);
                right_pos = System.Convert.ToInt32(signboard_results[2]);
                width = System.Convert.ToInt32(signboard_results[3]);
                height = System.Convert.ToInt32(signboard_results[4]);
                xlWorkSheet.Cells[xls_lastrow_idx, 1] = xls_lastrow_idx - 1;
                xlWorkSheet.Cells[xls_lastrow_idx, 2] = road_class;   
                xlWorkSheet.Cells[xls_lastrow_idx, 3] = width * System.Convert.ToInt32(txt_Factor.Text) * 0.025;
                xlWorkSheet.Cells[xls_lastrow_idx, 4] = height * System.Convert.ToInt32(txt_Factor.Text) * 0.025;
                xls_lastrow_idx += 1;
                

            }
            else if(signboard_results.Length == 6)
            {
                road_class = signboard_results[0];
                left_pos = System.Convert.ToInt32(signboard_results[1]);
                right_pos = System.Convert.ToInt32(signboard_results[2]);
                width = System.Convert.ToInt32(signboard_results[3]);
                height = System.Convert.ToInt32(signboard_results[4]);
                distance = System.Convert.ToInt32(signboard_results[5]);  
                xlWorkSheet.Cells[xls_lastrow_idx, 1] = xls_lastrow_idx - 1;
                xlWorkSheet.Cells[xls_lastrow_idx, 2] = road_class;
               
                xlWorkSheet.Cells[xls_lastrow_idx, 3] = width * System.Convert.ToInt32(txt_Factor.Text) * 0.025;
                xlWorkSheet.Cells[xls_lastrow_idx, 4] = height * System.Convert.ToInt32(txt_Factor.Text) * 0.025;
                xlWorkSheet.Cells[xls_lastrow_idx, 5] = distance * System.Convert.ToInt32(txt_Factor.Text) * 0.025;
                xls_lastrow_idx += 1;

            }

            if ((signboard_results.Length == 5) || (signboard_results.Length == 6))
            {
                imgPreview_2.Source = imgPreview.Source;
            }
        }

        private void create_new_excel()
        {

            xlApp = new Microsoft.Office.Interop.Excel.Application();

            if (xlApp == null)
            {
                System.Windows.MessageBox.Show("Excel is not properly installed!!");
                return;
            }

            //Excel.Workbook xlWorkBook;
            //Excel.Worksheet xlWorkSheet;
            misValue = System.Reflection.Missing.Value;

            xlWorkBook = xlApp.Workbooks.Add(misValue);
            xlWorkSheet = (Excel.Worksheet)xlWorkBook.Worksheets.get_Item(1);

            xlWorkSheet.Cells[1, 1] = "No";
            xlWorkSheet.Cells[1, 2] = "Sign Board Name";
            xlWorkSheet.Cells[1, 3] = "Width (M)";
            xlWorkSheet.Cells[1, 4] = "Height (M)";
            xlWorkSheet.Cells[1, 5] = "Distance from Road (M)";
        }


        [System.Runtime.InteropServices.DllImport("gdi32.dll")]
        public static extern bool DeleteObject(IntPtr hObject);

        [HandleProcessCorruptedStateExceptions]
        [SecurityCritical]
        private BitmapSource Bitmap2BitmapImage(Bitmap bitmap)
        {
            IntPtr hBitmap = IntPtr.Zero;
            BitmapSource retval;
            try
            {
                hBitmap = bitmap.GetHbitmap();
                retval = (BitmapSource)Imaging.CreateBitmapSourceFromHBitmap(
                             hBitmap,
                             IntPtr.Zero,
                             Int32Rect.Empty,
                             BitmapSizeOptions.FromEmptyOptions());
            }
            catch
            {
                retval = null;
            }
            finally
            {
                if (hBitmap != IntPtr.Zero)
                    DeleteObject(hBitmap);
            }

            return retval;
        }

        string ImagePath = "";
        bool _isPlaying = false;

        EngineSharp.VehicleCounter Engine = null;
        private double lane_width, lane_width_factor;

        private void BtnOpenFile_Click(object sender, RoutedEventArgs e)
        {
            using (OpenFileDialog openFileDialog = new OpenFileDialog())
            {
                openFileDialog.Filter = "Video Sources|*.mp4;*.avi|All Files|*.*";
                if (openFileDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                {
                    ImagePath = openFileDialog.FileName;
                    //imgPreview.Source = new BitmapImage(new Uri(ImagePath));
                    txtFilePath.Text = ImagePath;
                }
            }
        }

        private void BtnStart_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                // Read file using StreamReader.Reads file line by line 
                StreamReader file = new StreamReader("C:\\NSV, GEOTRAN\\LAS\\las parameters.ini");

                lane_width_factor = Convert.ToDouble(file.ReadLine());
                file.ReadLine();
                camera_name = file.ReadLine();
                file.Close();

                //MessageBox.Show(lane_width_factor.ToString());
                //MessageBox.Show(camera_name);

                //var cameraNames = new List<string>();
                //using (var searcher = new ManagementObjectSearcher("SELECT * FROM Win32_PnPEntity WHERE (PNPClass = 'Image' OR PNPClass = 'Camera')"))
                //{
                //    foreach (var device in searcher.Get())
                //    {
                //        cameraNames.Add(device["Caption"].ToString());
                //        camera_name =  device.ToString();
                //    }
                //}

                if (Engine == null)
                    return;

                if (_isPlaying)
                    Engine.Stop();

                if (ImagePath != "")
                {
                    Engine.Start(ImagePath);
                }
                else
                {
                    //MessageBox.Show(camera_name);
                    Engine.Start(camera_name);
                }
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show("Error code - 283" + ex.Message);
            }
        }

        private void BtnStop_Click(object sender, RoutedEventArgs e)
        {
            Engine.Stop();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            //string xlsx_filename = xlsx_filename = root_dir + "\\" + "out.xls";

            //xlWorkBook.SaveAs(xlsx_filename, Excel.XlFileFormat.xlWorkbookNormal, misValue, misValue, misValue, misValue, Excel.XlSaveAsAccessMode.xlExclusive, misValue, misValue, misValue, misValue, misValue);
            //xlWorkBook.Close(true, misValue, misValue);
            //xlApp.Quit();

            Engine.Stop();
        }
    }
}
