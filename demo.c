void remember_network(network net)
{
    int i;
    int count = 0;
    // 遍历网络net的每一层
    for (i = 0; i < net.n; ++i) {
        layer l = net.layers[i];
        // yolov3中为yolo层，V2中为region层
        if (l.type == YOLO || l.type == REGION || l.type == DETECTION) {
            // memcpy函数原型：void *memcpy(void *dest, const void *src, size_t n);
            // memcpy指的是c和c++使用的内存拷贝函数，memcpy函数的功能是从源src所指
            // 的内存地址的起始位置开始拷贝n个字节到目标dest所指的内存地址的起始位置中。
            // demo_index的初始值为0，在detect_in_thread函数中维护其值
            memcpy(predictions[demo_index] + count, net.layers[i].output, sizeof(float) * l.outputs);
            count += l.outputs;
        }
    }
}

int size_network(network net)
{
    int i;
    int count = 0;
    // 遍历网络的每一层
    for (i = 0; i < net.n; ++i) {
        layer l = net.layers[i];
        // 在yolov3的配置文件中，yolo层作为输出层，没有region和detection层，其中yolo层的
        // outputs参数表示输出数据量，output存储了具体输出数据（预测框信息）
        if (l.type == YOLO || l.type == REGION || l.type == DETECTION) {
            // yolov3中一共有3个yolo层，所以这个cout应该为3个数的和，更为具体点就是：
            // count=13*13*3*85+26*26*3*85+52*52*3*85
            count += l.outputs;
        }
    }
    return count;
}

void *detect_in_threads(void *ptr)
{
    int i = 0, count = 0;
    float *X = det_s.data;
    network_predict(net, X);
    remember_network(net);

    // 计算二维数组l.outputs中各列元素的均值，指针回传计算结果
    mean_arrays(predictions, NFRAMES,demo_total, avg);

    for (i = 0; i < net.n; ++i) {
        layer l = net.layers[i];
        // 将平均后的预测值重新赋值给网络yolo层的output
        if (l.type == YOLO || l.type == REGION || l.type == DETECTION) {
            // memcpy函数原型：void *memcpy(void *dest, const void *src, size_t n);
            // memcpy指的是c和c++使用的内存拷贝函数，memcpy函数的功能是从源src所指
            // 的内存地址的起始位置开始拷贝n个字节到目标dest所指的内存地址的起始位置中。
            memcpy(l.output, avg + count, sizeof(float) * l.outputs);
            count += l.outputs;
        }
    }
    //l.output = avg;

    free_image(det_s);

    ipl_images[demo_index] = det_img;
    det_img = ipl_images[(demo_index + NFRAMES / 2 + 1) % NFRAMES];
    demo_index = (demo_index + 1) % NFRAMES;

    if (letter_box)
        dets = get_network_boxes(&net, in_img->width, in_img->height, demo_thresh, demo_thresh, 0, 1, &nboxes, 1); // letter box
    else
        dets = get_network_boxes(&net, net.w, net.h, demo_thresh, demo_thresh, 0, 1, &nboxes, 0); // resized

    return 0;
}
