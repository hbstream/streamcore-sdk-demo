// StreamCore WinForms demo process entrypoint.
//
// Program owns only process-level WinForms startup concerns; SDK interaction
// belongs to MainForm so autorun and interactive modes share one host path.
using System;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace StreamCore.Demo.WinForms
{
    internal static class Program
    {
        [STAThread]
        private static void Main()
        {
            try
            {
                Console.OutputEncoding = Encoding.UTF8;
            }
            catch (IOException)
            {
                // WinExe 在没有有效控制台句柄时忽略 stdout 编码设置。
            }
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }
    }
}
