#pragma once
#include <QImage>
#include <QRect>
// Include the top edge; only the side toolbar and bottom button margins are excluded.
inline QRect diaryCaptureArea() { return QRect(150,0,1174,1752); }
// The Ask button (inkDiary.qmd): one toolbar cell under the close button.
inline QRect diaryButtonRect() { return QRect(1292,112,112,112); }
// The Haiku button: the next toolbar cell below Ask.
inline QRect diaryPremiumButtonRect() { return QRect(1292,224,112,112); }
// The tools button (MCP proxy): the next toolbar cell below the Haiku button.
inline QRect diaryToolsButtonRect() { return QRect(1292,336,112,112); }
// Keep our own button out of the page photo, the ink search and the unchanged-page check.
inline void diaryMaskControls(QImage &screen) {
    for(const QRect &r:{diaryButtonRect(),diaryPremiumButtonRect(),diaryToolsButtonRect()})
        for(int y=r.top();y<=r.bottom()&&y<screen.height();++y)
            for(int x=r.left();x<=r.right()&&x<screen.width();++x)
                screen.setPixelColor(x,y,Qt::white);
}
#include <array>
#include <vector>
#include <algorithm>
// A pale, nearly uniform full-width/height line is a template rule.
// Keep darker strokes on that line, and never filter black rules/content.
inline int ruleThreshold(const std::array<int,256> &histogram, int length) {
    int mode=int(std::max_element(histogram.begin(),histogram.end())-histogram.begin());
    if(mode<100 || mode>=210) return 210;
    int uniform=0;
    for(int i=std::max(0,mode-8);i<=std::min(255,mode+8);++i) uniform+=histogram[i];
    return uniform*100>=length*90 ? mode-40 : 210;
}
inline int lastInkRow(const QImage &source) {
    if(source.isNull()) return -1;
    QImage image=source.convertToFormat(QImage::Format_RGB32);
    std::vector<int> rows(image.height(),210),columns(image.width(),210);
    std::vector<std::array<int,256>> colHist(image.width());
    for(int y=0;y<image.height();++y) {
        std::array<int,256> hist{};
        const QRgb *pixels=reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for(int x=0;x<image.width();++x) {int gray=qGray(pixels[x]);++hist[gray];++colHist[x][gray];}
        rows[y]=ruleThreshold(hist,image.width());
    }
    for(int x=0;x<image.width();++x) columns[x]=ruleThreshold(colHist[x],image.height());
    // An opaque plot can hide the middle of a template margin. Recognize
    // remaining long, thin pale segments rather than demanding a full-height
    // line. Keep this fallback vertical-only so pale handwritten text survives.
    std::vector<int> partial(image.width(),210);
    for(int x=0;x<image.width();++x) {
        if(columns[x]<210) continue;
        const auto &hist=colHist[x];
        int mode=100+int(std::max_element(hist.begin()+100,hist.begin()+210)-(hist.begin()+100));
        int count=0,run=0,longest=0;
        for(int y=0;y<image.height();++y) {
            int gray=qGray(image.pixel(x,y));
            if(std::abs(gray-mode)<=8) ++count;
            // Horizontal rule intersections darken the same vertical line.
            if(gray>=mode-60 && gray<=mode+8) {++run;longest=std::max(longest,run);}
            else run=0;
        }
        if(count*5>=image.height() && longest>=std::max(64,image.height()/10)) partial[x]=mode-40;
    }
    // A template margin is a thin line, not a shaded region or broad marker.
    for(int x=0;x<image.width();) {
        if(partial[x]==210) {++x;continue;}
        int end=x+1;while(end<image.width() && partial[end]<210)++end;
        if(end-x<=4)for(int i=x;i<end;++i)columns[i]=partial[i];
        x=end;
    }
    for(int y=image.height()-1;y>=0;--y) {
        const QRgb *pixels=reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for(int x=0;x<image.width();++x) {
            int threshold=std::min(rows[y],columns[x]);
            // Two translucent rules darken where they cross (e.g. 191 -> 143).
            if(rows[y]<210 && columns[x]<210)
                threshold=(rows[y]+40)*(columns[x]+40)/255-40;
            if(qGray(pixels[x])<threshold) return y;
        }
    }
    return -1;
}
