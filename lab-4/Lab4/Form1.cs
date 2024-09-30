using System;
using System.Drawing;
using System.Windows.Forms;

namespace Lab4
{
    public partial class Form1 : Form
    {
        private Timer timer;
        private bool isPaused = false;
        private Button btnPauseResume;
        private double angle = 0;
        private int centerX, centerY, radius;

        public Form1()
        {
            InitializeComponent();
            this.Text = "Коло рухається";
            this.ControlBox = false;
            this.ClientSize = new Size(400, 400);
            this.DoubleBuffered = true;

            btnPauseResume = new Button
            {
                Text = "Призупинити",
                Location = new Point(10, 10),
                AutoSize = true
            };
            btnPauseResume.Click += BtnPauseResume_Click;
            this.Controls.Add(btnPauseResume);

            timer = new Timer
            {
                Interval = 30
            };
            timer.Tick += Timer_Tick;
            timer.Start();

            centerX = this.ClientSize.Width / 2;
            centerY = this.ClientSize.Height / 2;
            radius = 100;
        }

        private void Timer_Tick(object sender, EventArgs e)
        {
            if (!isPaused)
            {
                angle += 0.05;
                if (angle > 2 * Math.PI)
                {
                    angle -= 2 * Math.PI;
                }
                this.Invalidate();
            }
        }

        private void BtnPauseResume_Click(object sender, EventArgs e)
        {
            isPaused = !isPaused;
            btnPauseResume.Text = isPaused ? "Продовжити" : "Призупинити";
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            int ballX = centerX + (int)(radius * Math.Cos(angle)) - 10;
            int ballY = centerY + (int)(radius * Math.Sin(angle)) - 10;
            e.Graphics.FillEllipse(Brushes.Red, ballX, ballY, 20, 20);
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }
    }
}
