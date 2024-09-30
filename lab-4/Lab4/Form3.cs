using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

namespace Lab4
{
    public partial class Form3 : Form
    {
        private Timer timer;
        private bool isPaused = false;
        private Button btnPauseResume;
        private float x = 0;
        private List<PointF> points;

        public Form3()
        {
            InitializeComponent();
            this.Text = "Синусоїда";
            this.ControlBox = false;
            this.ClientSize = new Size(800, 400);
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

            points = new List<PointF>();
        }

        private void Timer_Tick(object sender, EventArgs e)
        {
            if (!isPaused)
            {
                x += 0.5f;
                if (x > this.ClientSize.Width)
                {
                    x = 0;
                    points.Clear();
                }
                float y = (float)(Math.Sin(x * 0.05f) * 100 + this.ClientSize.Height / 2);
                points.Add(new PointF(x, y));
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
            if (points.Count > 1)
            {
                e.Graphics.DrawLines(Pens.Green, points.ToArray());
            }
        }

        private void Form3_Load(object sender, EventArgs e)
        {

        }
    }
}
