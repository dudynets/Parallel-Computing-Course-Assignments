using System;
using System.Threading;
using System.Windows.Forms;

namespace Lab4
{
    public partial class MainForm : Form
    {
        private Thread thread1, thread2, thread3, thread4;
        private CancellationTokenSource cts1, cts2, cts3, cts4;
        private bool isRunning1 = false, isRunning2 = false, isRunning3 = false, isRunning4 = false;
        private Form1 form1;
        private Form2 form2;
        private Form3 form3;
        private Form4 form4;

        public MainForm()
        {
            InitializeComponent();
            SetupUI();
        }

        private void SetupUI()
        {
            this.Text = "Лабораторна робота 4";
            this.ClientSize = new System.Drawing.Size(360, 200);

            Button btnThread1 = new Button() { Text = "Запустити потік 1", Location = new System.Drawing.Point(50, 30), AutoSize = true };
            btnThread1.Click += BtnThread1_Click;
            this.Controls.Add(btnThread1);

            Button btnThread2 = new Button() { Text = "Запустити потік 2", Location = new System.Drawing.Point(200, 30), AutoSize = true };
            btnThread2.Click += BtnThread2_Click;
            this.Controls.Add(btnThread2);

            Button btnThread3 = new Button() { Text = "Запустити потік 3", Location = new System.Drawing.Point(50, 100), AutoSize = true };
            btnThread3.Click += BtnThread3_Click;
            this.Controls.Add(btnThread3);

            Button btnThread4 = new Button() { Text = "Запустити потік 4", Location = new System.Drawing.Point(200, 100), AutoSize = true };
            btnThread4.Click += BtnThread4_Click;
            this.Controls.Add(btnThread4);
        }

        private void BtnThread1_Click(object sender, EventArgs e)
        {
            Button btn = sender as Button;
            if (!isRunning1)
            {
                cts1 = new CancellationTokenSource();
                thread1 = new Thread(() => Thread1Proc(cts1.Token));
                thread1.SetApartmentState(ApartmentState.STA);
                thread1.Start();
                isRunning1 = true;
                btn.Text = "Зупинити потік 1";
            }
            else
            {
                cts1.Cancel();
                CloseForm(form1);
                isRunning1 = false;
                btn.Text = "Запустити потік 1";
            }
        }

        private void Thread1Proc(CancellationToken token)
        {
            form1 = new Form1();
            form1.FormClosed += (s, e) => Application.ExitThread();
            Application.Run(form1);

            while (!token.IsCancellationRequested)
            {
                Thread.Sleep(50);
            }
        }

        private void BtnThread2_Click(object sender, EventArgs e)
        {
            Button btn = sender as Button;
            if (!isRunning2)
            {
                cts2 = new CancellationTokenSource();
                thread2 = new Thread(() => Thread2Proc(cts2.Token));
                thread2.SetApartmentState(ApartmentState.STA);
                thread2.Start();
                isRunning2 = true;
                btn.Text = "Зупинити потік 2";
            }
            else
            {
                cts2.Cancel();
                CloseForm(form2);
                isRunning2 = false;
                btn.Text = "Запустити потік 2";
            }
        }

        private void Thread2Proc(CancellationToken token)
        {
            form2 = new Form2();
            form2.FormClosed += (s, e) => Application.ExitThread();
            Application.Run(form2);

            while (!token.IsCancellationRequested)
            {
                Thread.Sleep(50);
            }
        }

        private void BtnThread3_Click(object sender, EventArgs e)
        {
            Button btn = sender as Button;
            if (!isRunning3)
            {
                cts3 = new CancellationTokenSource();
                thread3 = new Thread(() => Thread3Proc(cts3.Token));
                thread3.SetApartmentState(ApartmentState.STA);
                thread3.Start();
                isRunning3 = true;
                btn.Text = "Зупинити потік 3";
            }
            else
            {
                cts3.Cancel();
                CloseForm(form3);
                isRunning3 = false;
                btn.Text = "Запустити потік 3";
            }
        }

        private void Thread3Proc(CancellationToken token)
        {
            form3 = new Form3();
            form3.FormClosed += (s, e) => Application.ExitThread();
            Application.Run(form3);
            
            while (!token.IsCancellationRequested)
            {
                Thread.Sleep(50);
            }
        }

        private void BtnThread4_Click(object sender, EventArgs e)
        {
            Button btn = sender as Button;
            if (!isRunning4)
            {
                cts4 = new CancellationTokenSource();
                thread4 = new Thread(() => Thread4Proc(cts4.Token));
                thread4.SetApartmentState(ApartmentState.STA);
                thread4.Start();
                isRunning4 = true;
                btn.Text = "Зупинити потік 4";
            }
            else
            {
                cts4.Cancel();
                CloseForm(form4);
                isRunning4 = false;
                btn.Text = "Запустити потік 4";
            }
        }

        private void Thread4Proc(CancellationToken token)
        {
            form4 = new Form4();
            form4.FormClosed += (s, e) => Application.ExitThread();
            Application.Run(form4);
            
            while (!token.IsCancellationRequested)
            {
                Thread.Sleep(50);
            }
        }

        private void CloseForm(Form form)
        {
            if (form != null && !form.IsDisposed)
            {
                form.Invoke(new Action(() =>
                {
                    form.Close();
                }));
            }
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
        }
    }
}
