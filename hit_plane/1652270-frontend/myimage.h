#ifndef MYIMAGE_H
#define MYIMAGE_H

static const QString myImage = QStringLiteral("data:image/jpeg;base64,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");

#endif // MYIMAGE_H
