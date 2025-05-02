#pragma once

namespace VirtualBenchmark
{

    struct Shape
    {
        virtual void Move(double x, double y)=0;
        virtual void Draw()=0;
    };

    struct Modify
    {
        // take slice
        // cut in half
    };

    // modify
    // Take Slice
    // Cut in half
    // Cut into quarters

    // render
    // Draw

    // transform
    // Move
    // scale

    // rotate
    // rotate
    


    struct Circle : Shape
    {
        void Move(double x, double y) override;
        void Draw() override;
        double move = 0.0;
        double draw = 0.0;
    };

    struct Rectangle : Shape
    {
        void Move(double x, double y) override;
        void Draw() override;
        double move = 0.0;
        double draw = 0.0;
    };

}