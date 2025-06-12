#include <QApplication>
#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QClipboard>
#include <QDrag>
#include <QMimeData>
#include <QProcess>
#include <QMouseEvent>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <X11/Xlib.h>
#include <X11/keysym.h>

using json = nlohmann::json;

class DraggableListWidget : public QListWidget {
    Q_OBJECT
public:
    DraggableListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setSelectionMode(QAbstractItemView::SingleSelection);
        setDragEnabled(true);
        setAcceptDrops(false);
    }

protected:
    void mouseMoveEvent(QMouseEvent* event) override {
        if (!(event->buttons() & Qt::LeftButton)) {
            return;
        }
        QListWidgetItem* item = itemAt(event->pos());
        if (!item) {
            return;
        }
        startDrag(item);
    }

private:
    void startDrag(QListWidgetItem* item) {
        QString value = item->data(Qt::UserRole).toString();
        std::cerr << "Starting drag with value: " << value.toStdString() << std::endl;

        QMimeData* mimeData = new QMimeData;
        mimeData->setText(value);

        QDrag* drag = new QDrag(this);
        drag->setMimeData(mimeData);
        copyToClipboard(value);
        simulatePaste();
        drag->exec(Qt::CopyAction);
    }

    void copyToClipboard(const QString& value) {
        QClipboard* clipboard = QApplication::clipboard();
        if (clipboard) {
            clipboard->setText(value);
            std::cerr << "Copied to clipboard: " << value.toStdString() << std::endl;
        } else {
            std::cerr << "Failed to get clipboard" << std::endl;
        }
    }

    void simulatePaste() {
        Display* display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Error: Cannot open X display." << std::endl;
            return;
        }
        Window window = DefaultRootWindow(display);
        XKeyEvent event = {};
        event.display = display;
        event.window = window;
        event.root = window;
        event.subwindow = None;
        event.time = CurrentTime;
        event.same_screen = True;
        event.state = ControlMask;
        event.type = KeyPress;
        event.keycode = XKeysymToKeycode(display, XK_v);
        XSendEvent(display, window, True, KeyPressMask, (XEvent*)&event);
        event.type = KeyRelease;
        XSendEvent(display, window, True, KeyReleaseMask, (XEvent*)&event);
        event.keycode = XKeysymToKeycode(display, XK_Control_L);
        XSendEvent(display, window, True, KeyReleaseMask, (XEvent*)&event);
        XFlush(display);
        XCloseDisplay(display);
        std::cerr << "Simulated paste event" << std::endl;
    }
};

class EasyInfoDropWindow : public QMainWindow {
    Q_OBJECT
public:
    EasyInfoDropWindow(const json& config, QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("EasyInfoDrop");
        resize(200, 300);

        QWidget* centralWidget = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(centralWidget);
        setCentralWidget(centralWidget);

        listWidget = new DraggableListWidget(this);
        loadFields(config["items"]);
        connect(listWidget, &QListWidget::itemClicked, this, &EasyInfoDropWindow::onItemClicked);
        layout->addWidget(listWidget);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        pinButton = new QPushButton("Pin", this);
        connect(pinButton, &QPushButton::clicked, this, &EasyInfoDropWindow::toggleSticky);
        editButton = new QPushButton("Edit", this);
        connect(editButton, &QPushButton::clicked, this, &EasyInfoDropWindow::openConfig);
        buttonLayout->addWidget(pinButton);
        buttonLayout->addWidget(editButton);
        layout->addLayout(buttonLayout);

        std::cerr << "EasyInfoDropWindow initialized successfully" << std::endl;
    }

private slots:
    void onItemClicked(QListWidgetItem* item) {
        if (item) {
            QString value = item->data(Qt::UserRole).toString();
            std::cerr << "Item clicked, copying value: " << value.toStdString() << std::endl;
            copyToClipboard(value);
        } else {
            std::cerr << "No item provided to onItemClicked" << std::endl;
        }
    }

    void toggleSticky() {
        isSticky = !isSticky;
        Qt::WindowFlags flags = windowFlags();
        if (isSticky) {
            flags |= Qt::WindowStaysOnTopHint;
            pinButton->setText("Unpin");
        } else {
            flags &= ~Qt::WindowStaysOnTopHint;
            pinButton->setText("Pin");
        }
        setWindowFlags(flags);
        show();
        std::cerr << "Sticky toggled: " << (isSticky ? "Pinned" : "Unpinned") << std::endl;
    }

    void openConfig() {
        const char* editors[] = {"gedit", "kate", "nano", "notepad", nullptr};
        QString configPath = "config/config.json";
        for (const char* editor : editors) {
            if (QProcess::startDetached(QString::fromUtf8(editor), {configPath})) {
                std::cerr << "Opened config with " << editor << std::endl;
                return;
            }
        }
        std::cerr << "Error: No text editor found." << std::endl;
    }

private:
    void loadFields(const json& fields) {
        listWidget->clear();
        for (const auto& field : fields) {
            QString name = QString::fromStdString(field["name"].get<std::string>());
            QString value = QString::fromStdString(field["value"].get<std::string>());
            QListWidgetItem* item = new QListWidgetItem(name, listWidget);
            item->setData(Qt::UserRole, value);
            item->setToolTip(value);
            std::cerr << "Added item: " << name.toStdString() << " with value: " << value.toStdString() << std::endl;
        }
    }

    void copyToClipboard(const QString& value) {
        QClipboard* clipboard = QApplication::clipboard();
        if (clipboard) {
            clipboard->setText(value);
            std::cerr << "Copied to clipboard: " << value.toStdString() << std::endl;
        } else {
            std::cerr << "Failed to get clipboard" << std::endl;
        }
    }

    DraggableListWidget* listWidget;
    QPushButton* pinButton;
    QPushButton* editButton;
    bool isSticky = false;
};

#include "main.moc"

int main(int argc, char* argv[]) {
    json config;
    try {
        std::ifstream config_file("config/config.json");
        if (!config_file.is_open()) {
            std::cerr << "Error: config/config.json not found" << std::endl;
            return 1;
        }
        config = json::parse(config_file);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return 1;
    }

    QApplication app(argc, argv);
    EasyInfoDropWindow window(config);
    window.show();
    return app.exec();
}
